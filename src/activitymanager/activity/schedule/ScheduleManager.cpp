// Copyright (c) 2009-2018 LG Electronics, Inc.
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

#include <activity/schedule/ScheduleManager.h>
#include <stdexcept>

#include "util/Logging.h"
#include "service/BusConnection.h"

const char *ScheduleManager::kPowerdWakeupKey =
        "com.webos.service.activitymanager.wakeup";

ScheduleManager::ScheduleManager()
{
}

ScheduleManager::~ScheduleManager()
{
}

void ScheduleManager::scheduledWakeup()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Powerd wakeup callback received");

    wake();
}

void ScheduleManager::enable()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Powerd scheduler enabled");

    monitorSystemTime();
}

void ScheduleManager::updateTimeout(time_t nextWakeup, time_t curTime)
{
    MojObject params;
    MojErr err;
    MojErr errs = MojErrNone;
    char formattedTime[32];

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Updating powerd scheduling callback - nextWakeup %llu, current time %llu",
                 (unsigned long long )nextWakeup, (unsigned long long )curTime);

    formatWakeupTime(nextWakeup, formattedTime, sizeof(formattedTime));

    err = params.putBool("wakeup", true);
    MojErrAccumulate(errs, err);

    err = params.putString(_T("at"), formattedTime);
    MojErrAccumulate(errs, err);

    err = params.putString(_T("key"), kPowerdWakeupKey);
    MojErrAccumulate(errs, err);

    err = params.putString(_T("uri"),
                           "luna://com.webos.service.activitymanager/callback/scheduledwakeup");
    MojErrAccumulate(errs, err);

    MojObject cbParams(MojObject::TypeObject);
    err = params.put(_T("params"), cbParams);
    MojErrAccumulate(errs, err);

    if (errs) {
        LOG_AM_ERROR(MSGID_SET_TIMEOUT_PARAM_ERR, 0,
                     "Error constructing parameters for powerd set timeout call");
        throw std::runtime_error("Error constructing parameters for powerd set timeout call");
    }

    m_call = std::make_shared<LunaPtrCall<ScheduleManager>>(
            this,
            &ScheduleManager::handleTimeoutSetResponse,
            true,
            "luna://com.webos.service.sleep/timeout/set",
            params);
    m_call->call();
}

void ScheduleManager::cancelTimeout()
{
    MojObject params;
    MojErr err = params.putString(_T("key"), kPowerdWakeupKey);

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Cancelling powerd timeout");

    if (err) {
        LOG_AM_ERROR(MSGID_CLEAR_TIMEOUT_PARAM_ERR, 0,
                     "Error constructing parameters for powerd clear timeout call");
        throw std::runtime_error("Error constructing parameters for powerd clear timeout call");
    }

    m_call = std::make_shared<LunaPtrCall<ScheduleManager>>(
            this,
            &ScheduleManager::handleTimeoutClearResponse,
            true,
            "luna://com.webos.service.sleep/timeout/clear",
            params);
    m_call->call();
}

void ScheduleManager::monitorSystemTime()
{
    MojObject params;
    MojErr err = params.putBool(_T("subscribe"), true);

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Subscribing to System Time change notifications");

    if (err) {
        LOG_AM_ERROR(MSGID_GET_SYSTIME_PARAM_ERR, 0,
                     "Error constructing parameters for subscription to System Time Service");
        throw std::runtime_error(
                "Error constructing parameters for subscription to System Time Service");
    }

    m_systemTimeCall = std::make_shared<LunaPtrCall<ScheduleManager>>(
            this,
            &ScheduleManager::handleSystemTimeResponse,
            true,
            "luna://com.webos.service.systemservice/time/getSystemTime",
            params,
            LunaCall::kUnlimited);
    m_systemTimeCall->call();
}

size_t ScheduleManager::formatWakeupTime(time_t wake, char *at, size_t len) const
{
    /* "MM/DD/YY HH:MM:SS" (UTC) */

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    gmtime_r(&wake, &tm);
    return strftime(at, len, "%m/%d/%Y %H:%M:%S", &tm);
}

void ScheduleManager::handleTimeoutSetResponse(MojServiceMessage *msg,
                                               const MojObject& response,
                                               MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Timeout Set response %s", MojoObjectJson(response).c_str());

    if (err != MojErrNone) {
        if (LunaCall::isPermanentFailure(msg, response, err)) {
            LOG_AM_WARNING(MSGID_SCH_WAKEUP_REG_ERR, 0,
                           "Failed to register scheduled wakeup: %s",
                           MojoObjectJson(response).c_str());
        } else {
            LOG_AM_WARNING(MSGID_SCH_WAKEUP_REG_RETRY, 0,
                           "Failed to register scheduled wakeup, retrying: %s",
                           MojoObjectJson(response).c_str());
            m_call->call();
            return;
        }
    } else {
        LOG_AM_DEBUG("Successfully registered scheduled wakeup");
    }

    m_call.reset();
}

void ScheduleManager::handleTimeoutClearResponse(MojServiceMessage *msg,
                                                 const MojObject& response,
                                                 MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Timeout Clear response %s", MojoObjectJson(response).c_str());

    if (err != MojErrNone) {
        if (LunaCall::isPermanentFailure(msg, response, err)) {
            LOG_AM_WARNING(MSGID_SCH_WAKEUP_CANCEL_ERR, 0,
                           "Failed to cancel scheduled wakeup: %s",
                           MojoObjectJson(response).c_str());
        } else {
            LOG_AM_WARNING(MSGID_SCH_WAKEUP_CANCEL_RETRY, 0,
                           "Failed to cancel scheduled wakeup, retrying: %s",
                           MojoObjectJson(response).c_str());
            m_call->call();
            return;
        }
    } else {
        LOG_AM_DEBUG("Successfully cancelled scheduled wakeup");
    }

    m_call.reset();
}

void ScheduleManager::handleSystemTimeResponse(MojServiceMessage *msg,
                                               const MojObject& response,
                                               MojErr err)
{
    MojInt64 localOffset;

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("System Time response %s", MojoObjectJson(response).c_str());

    if (err != MojErrNone) {
        if (LunaCall::isPermanentFailure(msg, response, err)) {
            LOG_AM_WARNING(MSGID_SYS_TIME_RSP_FAIL, 0,
                           "System Time subscription experienced an uncorrectable failure: %s",
                           MojoObjectJson(response).c_str());
            m_systemTimeCall.reset();
        } else {
            LOG_AM_WARNING(MSGID_GET_SYSTIME_RETRY, 0,
                           "System Time subscription failed, retrying: %s",
                           MojoObjectJson(response).c_str());
            monitorSystemTime();
        }
        return;
    }

    bool found = response.get(_T("offset"), localOffset);
    if (!found) {
        LOG_AM_WARNING(MSGID_SYSTIME_NO_OFFSET, 0,
                       "System Time message is missing timezone offset");
    } else {
        LOG_AM_DEBUG("System Time timezone offset: %lld",
                     (long long) localOffset);

        localOffset *= 60;

        setLocalOffset((off_t) localOffset);
    }

    /* The time changed response can also trip if the actual time(2) has
     * been updated, which should cause a recalculation also of the
     * interval schedules (at least if 'skip' is set) */
    timeChanged();
}

