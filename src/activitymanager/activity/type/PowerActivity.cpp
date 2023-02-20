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

#include "PowerActivity.h"

#include <sstream>

#include "activity/Activity.h"
#include "util/Logging.h"
#include "service/BusConnection.h"

const int PowerActivity::kPowerActivityLockDuration = 900;
const int PowerActivity::kPowerActivityLockUpdateInterval = 900;

const int PowerActivity::kPowerActivityDebounceDuration = 12;

PowerActivity::PowerActivity(std::shared_ptr<Activity> activity,
                             unsigned long serial)
    : AbstractPowerActivity(activity)
    , m_serial(serial)
    , m_currentState(kPowerUnlocked)
    , m_targetState(kPowerUnlocked)
{
}

PowerActivity::~PowerActivity()
{
    if (m_currentState != kPowerUnlocked) {
        LOG_AM_DEBUG("Power Activity serial %lu destroyed while power is not unlocked", m_serial);
    }
}

AbstractPowerActivity::PowerState PowerActivity::getPowerState() const
{
    return m_currentState;
}

void PowerActivity::begin()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    auto activity_ptr = m_activity.lock();
    if (!activity_ptr) {
        return;
    }
    LOG_AM_DEBUG("[Activity %llu] Locking power on", activity_ptr->getId());

    if ((m_currentState == kPowerLocked) || (m_targetState == kPowerLocked)) {
        return;
    }

    if (m_timeout) {
        m_timeout.reset();
    }

    /* If there's a command in flight, cancel it. */
    if (m_call) {
        m_call.reset();
    }

    m_targetState = kPowerLocked;
    m_currentState = kPowerUnknown;

    MojErr err = createRemotePowerActivity();
    if (err) {
        LOG_AM_ERROR(MSGID_PWR_LOCK_CREATE_FAIL, 1,
                     PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                     "Failed to issue command to create power lock");

        /* Fake it so the Activity doesn't stall */
        LOG_AM_WARNING(MSGID_PWR_LOCK_FAKE_NOTI, 1,
                       PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                       "Faking power locked notification");

        m_currentState = kPowerLocked;
        activity_ptr->powerLockedNotification();
    }
}

void PowerActivity::end()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    auto activity_ptr = m_activity.lock();
    if (!activity_ptr) {
        return;
    }
    LOG_AM_DEBUG("[Activity %llu] Unlocking power", activity_ptr->getId());

    if ((m_currentState == kPowerUnlocked) || (m_targetState == kPowerUnlocked)) {
        return;
    }

    if (m_timeout) {
        m_timeout.reset();
    }

    /* If there's a command in flight, cancel it. */
    if (m_call) {
        m_call.reset();
    }

    m_targetState = kPowerUnlocked;
    m_currentState = kPowerUnknown;

    bool debounce = activity_ptr->isPowerDebounce();

    MojErr err;
    if (debounce) {
        err = createRemotePowerActivity(true);
    } else {
        err = removeRemotePowerActivity();
    }

    if (err) {
        LOG_AM_ERROR(MSGID_PWR_UNLOCK_CREATE_FAIL, 1,
                     PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                     "Failed to issue command to %s power unlock",
                     debounce ? "debounce" : "remove");

        /* Fake it so the Activity doesn't stall - the lock will fall off
         * on its own... not that that's a good thing, but better than
         * nothing. */
        LOG_AM_WARNING(MSGID_PWR_UNLOCK_FAKE_NOTI, 1,
                       PMLOGKFV("Activity","%llu",activity_ptr->getId()),
                       "Faking power unlocked notification");

        m_currentState = kPowerUnlocked;
        activity_ptr->powerUnlockedNotification();
    }
}

unsigned long PowerActivity::getSerial() const
{
    return m_serial;
}

void PowerActivity::powerLockedNotification(MojServiceMessage *msg,
                                            const MojObject& response,
                                            MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    auto activity_ptr = m_activity.lock();
    if (!activity_ptr) {
        return;
    }

    if (err == MojErrNone) {
        if (m_currentState != kPowerLocked) {
            LOG_AM_DEBUG("[Activity %llu] Power lock successfully created",
                         activity_ptr->getId());

            m_currentState = kPowerLocked;
            activity_ptr->powerLockedNotification();
        } else {
            LOG_AM_DEBUG("[Activity %llu] Power lock successfully updated",
                         activity_ptr->getId());
        }

        if (!m_timeout) {
            m_timeout =
                    std::make_shared<Timeout<PowerActivity>>(
                            std::dynamic_pointer_cast<PowerActivity, AbstractPowerActivity>(shared_from_this()),
                            kPowerActivityLockUpdateInterval,
                            &PowerActivity::timeoutNotification);
        }

        m_timeout->arm();

        m_call.reset();
        return;
    }

    if (LunaCall::isPermanentBusFailure(msg, response, err)) {
        LOG_AM_WARNING(MSGID_PWRLK_NOTI_CREATE_FAIL, 1,
                       PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                       "Attempt to create power lock failed. Error %s",
                       MojoObjectJson(response).c_str());
        m_call.reset();
        return;
    }

    if (m_currentState != kPowerLocked) {
        LOG_AM_WARNING(MSGID_PWRLK_NOTI_CREATE_FAIL, 1,
                       PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                       "Attempt to create power lock failed, retrying. Error %s",
                       MojoObjectJson(response).c_str());
    } else {
        LOG_AM_WARNING(MSGID_PWRLK_NOTI_UPDATE_FAIL, 1,
                       PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                       "Attempt to update power lock failed, retrying. Error %s",
                       MojoObjectJson(response).c_str());
    }

    m_call.reset();

    /* Retry - powerd may have restarted. */
    MojErr err2 = createRemotePowerActivity();
    if (err2) {
        LOG_AM_WARNING(MSGID_PWR_LOCK_CREATE_FAIL, 1,
                       PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                       "Failed to issue command to create power lock in Noti");

        /* If power was not currently locked, fake the create so the
         * Activity doesn't hang */
        if (m_currentState != kPowerLocked) {
            LOG_AM_WARNING(MSGID_PWR_LOCK_FAKE_NOTI, 1,
                           PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                           "Faking power locked notification in noti");

            m_currentState = kPowerLocked;
            activity_ptr->powerLockedNotification();
        }
    }
}

void PowerActivity::powerUnlockedNotification(MojServiceMessage *msg,
                                              const MojObject& response,
                                              MojErr err,
                                              bool debounce)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    auto activity_ptr = m_activity.lock();
    if (!activity_ptr) {
        return;
    }

    if (err == MojErrNone) {
        LOG_AM_DEBUG("[Activity %llu] Power lock successfully %s",
                     activity_ptr->getId(),
                     debounce ? "debounced" : "removed");

        // reset the call *before* the unlocked notification; if
        // we end up requeuing the activity due to a transient, we may end
        // up sending a new call.
        m_call.reset();

        m_currentState = kPowerUnlocked;
        activity_ptr->powerUnlockedNotification();
        return;
    }

    if (LunaCall::isPermanentBusFailure(msg, response, err)) {
        LOG_AM_WARNING(MSGID_PWRULK_NOTI_ERR, 1,
                       PMLOGKFV("Activity","%llu",activity_ptr->getId()),
                       "Attempt to %s power lock failed. Error: %s",
                       debounce ? "debounce" : "remove",
                       MojoObjectJson(response).c_str());
        m_call.reset();
        return;
    }

    LOG_AM_WARNING(MSGID_PWRULK_NOTI_ERR, 1,
                   PMLOGKFV("Activity","%llu",activity_ptr->getId()),
                   "Attempt to %s power lock failed, retrying. Error: %s",
                   debounce ? "debounce" : "remove",
                   MojoObjectJson(response).c_str());

    m_call.reset();

    /* Retry. XXX but if error is "Activity doesn't exist", it's done. */
    MojErr err2;
    if (debounce) {
        err2 = createRemotePowerActivity(true);
    } else {
        err2 = removeRemotePowerActivity();
    }

    if (err2) {
        LOG_AM_WARNING(MSGID_PWR_UNLOCK_CREATE_ERR, 1,
                       PMLOGKFV("Activity","%llu",activity_ptr->getId()),
                       "Failed to issue command to %s power lock",
                       debounce ? "debounce" : "remove");

        /* Not much to do at this point, let the Activity move on
         * so it doesn't hang. */
        LOG_AM_WARNING(MSGID_PWR_UNLOCK_FAKE_NOTI, 1,
                       PMLOGKFV("Activity","%llu",activity_ptr->getId()),
                       "Faking power unlocked notification in Noti");

        m_currentState = kPowerUnlocked;
        activity_ptr->powerUnlockedNotification();
    }
}

void PowerActivity::powerUnlockedNotificationNormal(MojServiceMessage *msg,
                                                    const MojObject& response,
                                                    MojErr err)
{
    powerUnlockedNotification(msg, response, err, false);
}

void PowerActivity::powerUnlockedNotificationDebounce(MojServiceMessage *msg,
                                                      const MojObject& response,
                                                      MojErr err)
{
    powerUnlockedNotification(msg, response, err, true);
}

void PowerActivity::timeoutNotification()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    auto activity_ptr = m_activity.lock();
    if (!activity_ptr) {
        return;
    }
    LOG_AM_DEBUG("[Activity %llu] Attempting to update power lock", activity_ptr->getId());

    LOG_AM_WARNING(MSGID_PWR_TIMEOUT_NOTI, 2,
                   PMLOGKFV("Activity", "%llu", activity_ptr->getId()),
                   PMLOGKFV("LOCKDURATION", "%d", kPowerActivityLockDuration),
                   "Power Activity exceeded maximum length of seconds, not being renewed");
}

std::string PowerActivity::getRemotePowerActivityName() const
{
    std::shared_ptr<Activity> activity = m_activity.lock();
    std::stringstream name;

    /* am-<creator>-<activityId>.<serial> */
    name << "am:" << activity->getCreator().getId() << "-" << activity->getId() << "." << m_serial;

    return name.str();
}

MojErr PowerActivity::createRemotePowerActivity(bool debounce)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojObject params;

    err = params.putString(_T("id"), getRemotePowerActivityName().c_str());
    MojErrCheck(err);

    int duration;
    if (debounce) {
        duration = kPowerActivityDebounceDuration * 1000;
    } else {
        duration = kPowerActivityLockDuration * 1000;
    }

    err = params.putInt(_T("duration_ms"), duration);
    MojErrCheck(err);

    m_call = std::make_shared<LunaWeakPtrCall<PowerActivity>>(
            std::dynamic_pointer_cast<PowerActivity, AbstractPowerActivity>(shared_from_this()),
            (debounce ? &PowerActivity::powerUnlockedNotificationDebounce : &PowerActivity::powerLockedNotification),
            true,
            "luna://com.webos.service.sleep/com/palm/power/activityStart",
            params);
    m_call->call();

    return MojErrNone;
}

MojErr PowerActivity::removeRemotePowerActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojObject params;

    err = params.putString(_T("id"), getRemotePowerActivityName().c_str());
    MojErrCheck(err);

    m_call = std::make_shared<LunaWeakPtrCall<PowerActivity>>(
            std::dynamic_pointer_cast<PowerActivity, AbstractPowerActivity>(shared_from_this()),
            &PowerActivity::powerUnlockedNotificationNormal,
            true,
            "luna://com.webos.service.sleep/com/palm/power/activityEnd",
            params);
    m_call->call();

    return MojErrNone;
}

