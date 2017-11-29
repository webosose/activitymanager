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

#include "ContinuousActivity.h"

ContinuousActivity::ContinuousActivity(activityId_t id)
    : Activity(id)
    , m_shouldWait(false)
{
}

ContinuousActivity::~ContinuousActivity()
{
    LOG_AM_DEBUG("[ContinuousActivity %llu] Cleaning up", m_id);
}

bool ContinuousActivity::isRunnable() const
{
    if (m_shouldWait || m_activitySnapshots.size() == 0) {
        return false;
    }

    return true;
}

bool ContinuousActivity::isShouldWait() const
{
    if (m_activitySnapshots.size() > 1) {
        return true;
    } else {
        return false;
    }
}

void ContinuousActivity::onSuccessTrigger(std::shared_ptr<ITrigger> trigger, bool statusChanged, bool valueChanged)
{
    if (!valueChanged) {
        return;
    }

    LOG_AM_DEBUG("[ContinuousActivity %llu] Trigger \"%s\" %s", m_id, trigger->getName().c_str(),
                 trigger->isSatisfied() ? "Satisfiy" : "Unsatisfy");

    respondForStart();

    if (pushToWaitQueue() && isShouldWait()) {
        m_shouldWait = true;
    }

    if (statusChanged || Activity::isRunnable()) {
        m_state->onTriggerUpdate(shared_from_this(), trigger);
    }
}

void ContinuousActivity::onMetRequirement(std::string name)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[ContinuousActivity %llu] [Requirement %s] met", m_id, name.c_str());

    auto it = m_requirements.find(name);
    if (it == m_requirements.end()) {
        LOG_AM_ERROR(MSGID_REQ_MET_OWNER_MISMATCH, 2,
                     PMLOGKS("Requirement", name.c_str()),
                     PMLOGKFV("Activity", "%llu", m_id),
                     "Met requirement is not owned by activity");
        throw std::runtime_error("Requirement owner mismatch");
    }

    if (pushToWaitQueue() && isShouldWait()) {
        m_shouldWait = true;
    }

    m_state->onRequirementUpdate(shared_from_this(), (*it).second);
}

void ContinuousActivity::scheduled()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[ContinuousActivity %llu] Scheduled", m_id);

    if (pushToWaitQueue() && isShouldWait()) {
        m_shouldWait = true;
    }

    m_state->onSchedule(shared_from_this(), m_schedule);
}

void ContinuousActivity::callbackSucceeded(std::shared_ptr<AbstractCallback> callback)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[ContinuousActivity %llu] Callback succeeded", getId());

    LOG_AM_INFO("CONTINUOUS_STEP", 3,
                PMLOGKFV("Activity", "%llu", m_id),
                PMLOGKS("State", m_state->getName().c_str()),
                PMLOGKFV("Queue", "%lu", m_activitySnapshots.size()),
                "onCallback");

    m_activitySnapshots.pop_front();
    m_shouldWait = false;

    m_state->onCallbackSucceed(shared_from_this(), callback);
}

void ContinuousActivity::doCallback()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[ContinuousActivity %llu] Attempting to generate callback", m_id);

    if (!m_callback)
        throw std::runtime_error("No callback exists for Activity");

    MojObject& activityInfo = m_activitySnapshots.front();
    MojErr err = m_callback->call(activityInfo);
    if (err) {
        LOG_AM_ERROR(MSGID_ACTIVITY_CB_FAIL, 1,
                     PMLOGKFV("Activity", "%llu", m_id),
                     "Attempt to call callback failed");
        throw std::runtime_error("Failed to call Activity Callback");
    }
}

void ContinuousActivity::unscheduleActivity()
{
    if (m_schedule) {
        m_schedule->informActivityFinished();
        m_schedule->queue();
    }
}

bool ContinuousActivity::pushToWaitQueue()
{
    if (!Activity::isRunnable()) {
        return false;
    }

    MojObject activityInfo;
    if (MojErrNone != activityInfoToJson(activityInfo)) {
        return false;
    }

    m_activitySnapshots.push_back(activityInfo);

    LOG_AM_INFO("CONTINUOUS_STEP", 3,
                PMLOGKFV("Activity", "%llu", m_id),
                PMLOGKS("State", m_state->getName().c_str()),
                PMLOGKFV("Queue", "%lu", m_activitySnapshots.size()),
                "%s", (m_activitySnapshots.size() > 1) ? " (wait)" : "");
    return true;
}
