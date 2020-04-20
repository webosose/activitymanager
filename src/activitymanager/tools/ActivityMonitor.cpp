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

#include <activity/schedule/AbstractScheduleManager.h>
#include <conf/Setting.h>
#include "ActivityMonitor.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <fstream>

#include "activity/Activity.h"
#include "activity/ActivityManager.h"
#include "util/Logging.h"

ActivityMonitor::ActivityMonitor()
    : m_pipeReq(-1)
    , m_pipeResp(-1)
    , m_isMonitorRunning(false)
    , m_channel(NULL)
{
}

ActivityMonitor::~ActivityMonitor()
{
    closeChannel();
}

ActivityMonitor& ActivityMonitor::getInstance()
{
    static ActivityMonitor _instance;
    return _instance;
}

void ActivityMonitor::closeChannel()
{
    LOG_AM_INFO("MONITOR_INFO", 0, "close");
    m_isMonitorRunning = false;

    if (m_channel) {
        g_io_channel_shutdown(m_channel, TRUE, NULL);
        m_channel = NULL;
    }

    if (m_pipeReq != -1) {
        close(m_pipeReq);
        m_pipeReq = -1;
    }
    if (m_pipeResp != -1) {
        close(m_pipeResp);
        m_pipeResp = -1;
    }
}

void ActivityMonitor::initialize()
{
    LOG_AM_INFO("MONITOR_INFO", 0, "initialize");

    if (g_mkdir_with_parents(AM_IPC_DIR, 0755) == -1) {
        LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to create dir: %s", strerror(errno));
        return;
    }

    if (mkfifo(AM_MONITOR_REQ_PIPE_PATH, 0600) < 0) {
        if (errno != EEXIST) {
            LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to create request pipe: %s", strerror(errno));
            return;
        }
    }

    if (mkfifo(AM_MONITOR_RESP_PIPE_PATH, 0600) < 0) {
        if (errno != EEXIST) {
            LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to create response pipe: %s", strerror(errno));
            return;
        }
    }

    if ((m_pipeReq = open(AM_MONITOR_REQ_PIPE_PATH, O_RDONLY | O_NONBLOCK)) == -1) {
        LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to open request pipe: %s", strerror(errno));
        return;
    }

    GIOChannel* channel = g_io_channel_unix_new(m_pipeReq);
    g_io_add_watch(channel, (GIOCondition)(G_IO_IN | G_IO_HUP), ActivityMonitor::onRead, this);
    g_io_channel_unref(channel);
    m_channel = channel;
}

gboolean ActivityMonitor::onRead(GIOChannel* channel, GIOCondition condition, gpointer data)
{
    std::ifstream file;
    std::string line;
    ActivityMonitor& self = ActivityMonitor::getInstance();

    if (condition & G_IO_HUP) {
        goto Reset;
    }

    file.open(AM_MONITOR_REQ_PIPE_PATH);
    std::getline(file, line);

    LOG_AM_INFO("MONITOR_INFO", 1, PMLOGKS("command", line.c_str()), "");

    if ("monitor" == line) {
        self.m_isMonitorRunning = true;
        g_io_add_watch(channel, (GIOCondition)(G_IO_IN | G_IO_HUP), ActivityMonitor::onRead, NULL);
        return G_SOURCE_REMOVE;
    }

    if (self.m_pipeResp == -1) {
        if ((self.m_pipeResp = open(AM_MONITOR_RESP_PIPE_PATH, O_WRONLY | O_NONBLOCK)) == -1) {
            LOG_AM_INFO("MONITOR_INFO", 0, "Open response pipe: %s", strerror(errno));
            usleep(1000); // try again
            if ((self.m_pipeResp = open(AM_MONITOR_RESP_PIPE_PATH, O_WRONLY | O_NONBLOCK)) == -1) {
                LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to open pipe: %s", strerror(errno));
                goto Reset;
            }
        }
    }

    for (auto activity : ActivityManager::getInstance().getActivities()) {
        pbnjson::JValue object = activity->toJson();
        std::string json = object.stringify() + "\n";

        if (write(self.m_pipeResp, json.c_str(), json.length()) == -1) {
            LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to write data: %s", strerror(errno));
        }
    }

Reset:
    self.closeChannel();
    self.initialize();
    return G_SOURCE_REMOVE;
}

bool ActivityMonitor::isMonitorRunning()
{
    return m_isMonitorRunning;
}

void ActivityMonitor::send(std::shared_ptr<Activity> activity)
{
    if (!isMonitorRunning()) {
        return;
    }

    if (m_pipeResp == -1) {
        LOG_AM_INFO("MONITOR_INFO", 0, "Pipe opening");
        if ((m_pipeResp = open(AM_MONITOR_RESP_PIPE_PATH, O_WRONLY | O_NONBLOCK)) == -1) {
            LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to open pipe: %s", strerror(errno));
            return;
        }
        LOG_AM_INFO("MONITOR_INFO", 0, "Pipe opened");
    }

    pbnjson::JValue object = activity->toJson();
    std::string json = object.stringify() + "\n";

    if (write(m_pipeResp, json.c_str(), json.length()) == -1) {
        LOG_AM_WARNING("MONITOR_FAIL", 0, "Failed to write data: %s", strerror(errno));
        closeChannel();
        initialize();
        return;
    }
}
