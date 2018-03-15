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

#include <activity/schedule/AbstractScheduleManager.h>
#include "Schedule.h"

#include <ctime>

#include "activity/Activity.h"
#include "conf/ActivityJson.h"
#include "ScheduleManager.h"
#include "util/Logging.h"

const time_t Schedule::kDayOne = (60*60*24);
const time_t Schedule::kUnbounded = -1;
const time_t Schedule::kNever = -1;

MojLogger Schedule::s_log(_T("activitymanager.schedule"));

Schedule::Schedule(std::shared_ptr<Activity> activity,
                   time_t start)
    : m_activity(activity)
    , m_start(start)
    , m_local(false)
    , m_scheduled(false)
{
}

Schedule::~Schedule()
{
}

time_t Schedule::getNextStartTime() const
{
    return m_start;
}

void Schedule::calcNextStartTime()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Not an interval schedule, so next start time not updated",
                 m_activity.lock()->getId());
}

time_t Schedule::getTime() const
{
    time_t curTime = time(NULL);

    /* Adjust back to local time, if required.  This may not be the same
     * adjustment that was in place when the scheduled Activity was first
     * scheduled, because the time zone may have changed while it was running,
     * or after it was scheduled but before it made it through the queue to
     * run.  Should be close enough though.  Generally the time does not
     * change... */

    if (m_local) {
        curTime += ScheduleManager::getInstance().getLocalOffset();
    }

    return curTime;
}

std::shared_ptr<Activity> Schedule::getActivity() const
{
    return m_activity.lock();
}

bool Schedule::operator<(const Schedule& rhs) const
{
    return getNextStartTime() < rhs.getNextStartTime();
}

void Schedule::queue()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Queueing with Scheduler", m_activity.lock()->getId());

    if (isQueued()) {
        unQueue();
    }

    calcNextStartTime();

    m_scheduled = false;
    ScheduleManager::getInstance().addItem(shared_from_this());
}

void Schedule::unQueue()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Unqueueing from Scheduler", m_activity.lock()->getId());

    ScheduleManager::getInstance().removeItem(shared_from_this());
    m_scheduled = false;
}

bool Schedule::isQueued() const
{
    return m_queueItem.is_linked();
}

void Schedule::scheduled()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Scheduled", m_activity.lock()->getId());

    m_scheduled = true;
    m_activity.lock()->scheduled();
}

bool Schedule::isScheduled() const
{
    return m_scheduled;
}

void Schedule::informActivityFinished()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Finished", m_activity.lock()->getId());
}

bool Schedule::shouldReschedule() const
{
    return false;
}

void Schedule::setLocal(bool local)
{
    m_local = local;
}

bool Schedule::isLocal() const
{
    return m_local;
}

bool Schedule::isInterval() const
{
    return false;
}

MojErr Schedule::toJson(MojObject& rep, unsigned long flags) const
{
    MojErr err;

    if (flags & ACTIVITY_JSON_CURRENT) {
        err = rep.putBool(_T("scheduled"), m_scheduled);
        MojErrCheck(err);
    }

    if (m_local) {
        err = rep.putBool(_T("local"), m_local);
        MojErrCheck(err);
    }

    if (m_start != kDayOne) {
        err = rep.putString(_T("start"),
            AbstractScheduleManager::timeToString(m_start, !m_local).c_str());
        MojErrCheck(err);
    }

    return MojErrNone;
}

