// Copyright (c) 2017-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "ActivitySendHandler.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>

#include <glib.h>
#include <pbnjson.hpp>

#include "activity/Activity.h"
#include "activity/ActivityManager.h"
#include "conf/Setting.h"
#include "requirement/RequirementManager.h"
#include "util/Logging.h"

ActivitySendHandler::ActivitySendHandler()
    : m_pipeReq(-1)
    , m_pipeResp(-1)
{
}

ActivitySendHandler::~ActivitySendHandler()
{
    close(m_pipeReq);
    close(m_pipeResp);
}

void ActivitySendHandler::initialize()
{
    if (g_mkdir_with_parents(AM_IPC_DIR, 0755) == -1) {
        LOG_AM_WARNING("AM_SEND_FAIL", 0, "Failed to create dir: %s", strerror(errno));
        return;
    }

    if (mkfifo(AM_SEND_REQ_PIPE_PATH, 0600) < 0) {
        if (errno != EEXIST) {
            LOG_AM_WARNING("AM_SEND_FAIL", 0, "Failed to create request pipe: %s", strerror(errno));
            return;
        }
    }

    if (mkfifo(AM_SEND_RESP_PIPE_PATH, 0600) < 0) {
        if (errno != EEXIST) {
            LOG_AM_WARNING("AM_SEND_FAIL", 0, "Failed to create response pipe: %s", strerror(errno));
            return;
        }
    }

    if ((m_pipeReq = open(AM_SEND_REQ_PIPE_PATH, O_RDONLY | O_NONBLOCK)) == -1) {
        LOG_AM_WARNING("AM_SEND_FAIL", 0, "Failed to open request pipe: %s", strerror(errno));
        return;
    }

    GIOChannel* channel = g_io_channel_unix_new(m_pipeReq);
    g_io_add_watch(channel, (GIOCondition)(G_IO_IN), ActivitySendHandler::onRead, this);
}

gboolean ActivitySendHandler::onRead(GIOChannel* channel, GIOCondition condition, gpointer data)
{
    ActivitySendHandler* self = reinterpret_cast<ActivitySendHandler*>(data);
    if (!data) {
        LOG_AM_WARNING("AM_SEND_FAIL", 0, "handler is null");
        return G_SOURCE_REMOVE;
    }

    std::ifstream file;
    std::string line;
    file.open(AM_SEND_REQ_PIPE_PATH);
    std::getline(file, line);
    pbnjson::JValue request = pbnjson::JDomParser::fromString(line);
    pbnjson::JValue response = pbnjson::Object();
    bool clear = false;
    std::string errorText;
    std::string payload;

    pbnjson::JValue requirements;
    pbnjson::JValue triggers;

    std::shared_ptr<Activity> activity;
    if (request.hasKey("id")) {
        activityId_t id = request["id"].asNumber<int64_t>();
        try {
            activity = ActivityManager::getInstance().getActivity(id);
        } catch (std::runtime_error& error) {
            errorText = error.what();
            goto Return;
        }
    }

    if (request.hasKey("clear")) {
        LOG_AM_DEBUG("[Activity %llu] AM_SEND clear set", activity->getId());
        clear = true;
    }

    requirements = request["requirements"];
    for (int i = 0; i < requirements.arraySize(); i++) {
        pbnjson::JValue requirement = requirements[i];
        LOG_AM_DEBUG("[Activity %llu] AM_SEND %s", activity->getId(),
                     requirement.stringify().c_str());
        for (JValue::KeyValue pair : requirement.children()) {
            std::string name = pair.first.asString();
            std::shared_ptr<IRequirement> req = activity->getRequirement(name);
            if (!req) {
                errorText = "Requirement " + name + " not found";
                goto Return;
            }
            if (clear) {
                req->unsetSatisfied();
                continue;
            }
            req->setSatisfied(pair.second.asBool());
        }
    }

    triggers = request["triggers"];
    for (int i = 0; i < triggers.arraySize(); i++) {
        pbnjson::JValue trigger = triggers[i];
        LOG_AM_DEBUG("[Activity %llu] AM_SEND %s", activity->getId(),
                     trigger.stringify().c_str());
        for (JValue::KeyValue pair : trigger.children()) {
            std::string name = pair.first.asString();
            std::shared_ptr<ITrigger> trigger = activity->getTrigger(name);
            if (!trigger) {
                errorText = "Trigger " + name + " not found";
                goto Return;
            }
            if (clear) {
                trigger->unsetSatisfied();
                continue;
            }
            trigger->setSatisfied(pair.second.asBool());
        }
    }

Return:

    if (errorText.empty()) {
        response.put("returnValue", true);
    } else {
        response.put("returnValue", false);
        response.put("errorText", errorText);
    }
    payload = response.stringify() + "\n";

    if ((self->m_pipeResp = open(AM_SEND_RESP_PIPE_PATH, O_RDWR | O_NONBLOCK)) == -1) {
        LOG_AM_WARNING("AM_SEND_FAIL", 0, "Failed to open response pipe: %s", strerror(errno));
    } else if (write(self->m_pipeResp, payload.c_str(), payload.length()) == -1) {
        LOG_AM_WARNING("AM_SEND_FAIL", 0, "Failed to write response pipe: %s", strerror(errno));
    }

    g_io_add_watch(channel, (GIOCondition)(G_IO_IN), ActivitySendHandler::onRead, self);
    return G_SOURCE_REMOVE;
}
