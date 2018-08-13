// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#include "PermissionManager.h"

#include <glib.h>
#include <luna/MojLunaMessage.h>
#include <boost/algorithm/string.hpp>

#include "activity/ActivityManager.h"
#include "service/BusConnection.h"
#include "util/Logging.h"

static const char* bus_service_name = "com.webos.service.bus";
static const char* bus_service_method = "isCallAllowed";
static const unsigned CallTimeoutSec = 5;

PermissionManager::PermissionManager()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
}

PermissionManager::~PermissionManager()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
}

MojErr PermissionManager::syncCheck(const std::string& requester, const std::string& url)
{
    MojRefCountedPtr<PermissionManager::SyncChecker> checker(
            new PermissionManager::SyncChecker());
    return checker->check(requester, url);
}

std::string PermissionManager::getRequester(MojRefCountedPtr<MojServiceMessage> msg)
{
    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg.get());
    if (!lunaMsg) {
        throw std::runtime_error("Can't generate requester string from non-Luna message");
    }

    const char *appId = lunaMsg->appId();
    const char *serviceId = lunaMsg->senderId();
    const char *requester = appId;
    if (!requester)
        requester = serviceId;
    if (!requester)
        return lunaMsg->senderAddress();

    if (appId && serviceId && boost::algorithm::starts_with(serviceId, appId))
        requester = serviceId;

    return requester;
}

PermissionManager::SyncChecker::SyncChecker()
        : m_slot(this, &SyncChecker::receiveResponse)
        , m_received(false)
        , m_timerId(0)
{
}

PermissionManager::SyncChecker::~SyncChecker()
{
}

MojErr PermissionManager::SyncChecker::check(const std::string& requester,
                                             const std::string& url)
{
    MojErr err;
    MojObject payload;

    err = payload.putString("uri", url.c_str());
    MojErrCheck(err);
    err = payload.putString("requester", requester.c_str());
    MojErrCheck(err);

    MojRefCountedPtr<MojServiceRequest> req;
    err = BusConnection::getInstance().getService().createRequest(req);
    MojErrCheck(err);

    err = req->send(m_slot, bus_service_name, bus_service_method, payload);
    MojErrCheck(err);

    m_timerId = g_timeout_add_seconds(CallTimeoutSec, &PermissionManager::SyncChecker::timeout, this);

    while (!m_received) {
        g_main_context_iteration(g_main_context_default(), true);
    }

    bool returnValue = false;
    bool allowed = false;

    err = m_response.getRequired("returnValue", returnValue);
    MojErrCheck(err);
    err = m_response.getRequired("allowed", allowed);
    MojErrCheck(err);

    if (!returnValue) {
        return MojErrLuna;
    }
    if (!allowed) {
        return MojErrAccessDenied;
    }

    return MojErrNone;
}

MojErr PermissionManager::SyncChecker::receiveResponse(MojServiceMessage *,
                                                       MojObject& response,
                                                       MojErr err)
{
    m_response = response;
    m_received = true;
    g_source_remove(m_timerId);

    return MojErrNone;
}

int PermissionManager::SyncChecker::timeout(void* ctx)
{
    PermissionManager::SyncChecker *manager = (PermissionManager::SyncChecker *) ctx;
    manager->m_received = true;

    return G_SOURCE_REMOVE;
}

void PermissionManager::checkAccessRight(
        std::string requester, std::vector<std::string> urls, PermissionCallback callback)
{
    m_queue.push_back(std::make_tuple(requester, urls, callback));

    g_timeout_add(0, &PermissionManager::processQueue, this);
}

int PermissionManager::processQueue(void* ctx)
{
    static bool isWorking = false;

    PermissionManager* self = static_cast<PermissionManager*>(ctx);
    if (!self) {
        LOG_AM_WARNING("ACG_CHECK_ERR", 0, "PermissionManager is is null");
        return G_SOURCE_REMOVE;
    }

    if (isWorking || self->m_queue.empty()) {
        return G_SOURCE_REMOVE;
    }

    isWorking = true;
    auto& item = self->m_queue.front();

    std::string& requester = std::get<0>(item);
    std::vector<std::string>& vec = std::get<1>(item);
    PermissionCallback& callback = std::get<2>(item);
    MojErr err = MojErrNone;
    std::string errorText = "' doesn't have rights to call callback/trigger";

    for (const std::string& method : vec) {
        if ((err = self->syncCheck(requester, method)) != MojErrNone) {
            callback(err, "'" + requester + errorText);
            goto Exit;
        }
    }

    for (const std::string& method : vec) {
        if ((err = self->syncCheck(ActivityManager::kServiceName, method)) != MojErrNone) {
            callback(err, "'" + (ActivityManager::kServiceName + errorText));
            goto Exit;
        }
    }

    callback(MojErrNone, "");

Exit:
    self->m_queue.pop_front();
    isWorking = false;

    return G_SOURCE_CONTINUE;
}
