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
#include "IntervalSchedule.h"

#include <boost/regex.hpp>
#include <sstream>
#include <cstdlib>

#include "activity/Activity.h"
#include "conf/ActivityJson.h"
#include "ScheduleManager.h"
#include "util/Logging.h"

IntervalSchedule::IntervalSchedule(std::shared_ptr<Activity> activity,
                                   time_t start, unsigned interval, time_t end)
        : Schedule(activity, start)
        , m_end(end)
        , m_interval(interval)
        , m_skip(false)
        , m_nextStart(kNever)
        , m_lastFinished(kNever)
{
}

IntervalSchedule::~IntervalSchedule()
{
}

time_t IntervalSchedule::getNextStartTime() const
{
    return m_nextStart;
}

/*
 * Default interval scheduling policy is to align the scheduling points at
 * each of the interval tick points from the start point of the schedule.
 *
 * If missed is set, and a tick or ticks was missed, the Scheduler will run
 * the Activity *once* to catch up, and then continue to run at the aligned
 * points. */
void IntervalSchedule::calcNextStartTime()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    time_t curTime = getTime();
    time_t start = getBaseStartTime();

    /* Find the next upcoming scheduling point */
    if (curTime > start) {
        time_t next = curTime - start;
        next /= m_interval;
        next += 1;

        m_nextStart = start + (next * m_interval);
    } else {
        m_nextStart = start;
    }

    /* If the schedule slipped, make it available to run immediately, this
     * time. */
    if (!m_skip) {
        /* Last finished could be in the future if there was a time zone
         * change.  If so, just ignore it.  (Below will be < 0 and not
         * trip) */
        if (m_lastFinished == kNever) {
            if ((m_start != kDayOne) && ((unsigned) (m_nextStart - m_start) > m_interval)) {
                m_nextStart = curTime;
            }
        } else if (((m_nextStart - m_lastFinished) > 0) &&
                ((unsigned) (m_nextStart - m_lastFinished) > m_interval)) {
            m_nextStart = curTime;
        }
    }

    LOG_AM_DEBUG("[Activity %lu] Next start time is %s",
                 (unsigned long )m_activity.lock()->getId(),
                 AbstractScheduleManager::timeToString(m_nextStart, !m_local).c_str());
}

time_t IntervalSchedule::getBaseStartTime() const
{
    return ScheduleManager::getInstance().getSmartBaseTime();
}

bool IntervalSchedule::isInterval() const
{
    return true;
}

void IntervalSchedule::setSkip(bool skip)
{
    m_skip = skip;
}

void IntervalSchedule::setLastFinishedTime(time_t finished)
{
    /* Only finished times in the past, and after when the
     * event is supposed to have started running, are accepted. */
    if (((getTime() - finished) > 0) && ((finished - m_start) > 0)) {
        m_lastFinished = finished;
    }
}

void IntervalSchedule::informActivityFinished()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    time_t lastFinished = getTime();

    LOG_AM_DEBUG("[Activity %lu] Finished at %llu",
                 (unsigned long) m_activity.lock()->getId(),
                 (unsigned long long) lastFinished);

    /* Because the timezone can change to move time backwards, it's possible
     * for the current time to move *before* the scheduled time if this
     * happens while the event is running.  Don't let that happen, but
     * otherwise let it move where it wants.  It's for protection or further
     * relative scheduling so we want it to be as accurate as possible for
     * the current time on the device */
    if ((lastFinished - m_start) > 0) {
        m_lastFinished = lastFinished;
    }
}

bool IntervalSchedule::shouldReschedule() const
{
    if (m_end == kUnbounded) {
        return true;
    } else {
        return ((unsigned) (m_end - m_nextStart) > m_interval);
    }
}

MojErr IntervalSchedule::toJson(MojObject& rep, unsigned long flags) const
{
    MojErr err;

    if (flags & ACTIVITY_JSON_CURRENT) {
        err = rep.putString(_T("nextStart"), AbstractScheduleManager::timeToString(m_nextStart, !m_local).c_str());
        MojErrCheck(err);
    }

    if (m_end != kUnbounded) {
        err = rep.putString(_T("end"), AbstractScheduleManager::timeToString(m_end, !m_local).c_str());
        MojErrCheck(err);
    }

    if (m_skip) {
        err = rep.putBool(_T("skip"), true);
        MojErrCheck(err);
    }

    if (m_lastFinished != kNever) {
        err = rep.putString(_T("lastFinished"), AbstractScheduleManager::timeToString(m_lastFinished, !m_local).c_str());
        MojErrCheck(err);
    }

    err = rep.putString(_T("interval"), intervalToString(m_interval).c_str());
    MojErrCheck(err);

    return Schedule::toJson(rep, flags);
}

unsigned IntervalSchedule::stringToInterval(const char *intervalStr, bool smart)
{
    unsigned totalSecs = 0;
    unsigned total = 0;

    static const unsigned allowed[] = { 1, 5, 10, 15, 20, 30, 60, 180, 360, 720 };
    static const int num_allowed = (sizeof(allowed) / sizeof(int));

    int i;
    static const boost::regex durationRegex(
            "(?:(\\d+)D)?(?:(\\d+)H)?(?:(\\d+)M)?(?:(\\d+)S)?",
            boost::regex::icase | boost::regex::optimize);

    boost::cmatch what;

    if (!boost::regex_match(intervalStr, what, durationRegex)) {
        throw std::runtime_error("Failed to parse scheduling interval");
    }

    int days = 0, hours = 0, minutes = 0, seconds = 0;

    if (what[1].length()) {
        days = atoi(what[1].first);
    }
    if (what[2].length()) {
        hours = atoi(what[2].first);
    }
    if (what[3].length()) {
        minutes = atoi(what[3].first);
    }
    if (what[4].length()) {
        seconds = atoi(what[4].first);
    }

    totalSecs = seconds + (minutes * 60) + (hours * 3600) + (days * 86400);

    if (totalSecs == 0) {
        throw std::runtime_error("Duration must be non-zero");
    }

    if (!smart) {
        return totalSecs;
    }

    if ((totalSecs % 60) != 0) {
        throw std::runtime_error("Only durations of even minutes may be specified");
    }

    total = totalSecs / 60;
    for (i = 0; i < num_allowed ; i++) {
        if (total == allowed[i])
            break;
    }

    if (i < num_allowed) {
        /* Explicitly in the allowed list */
        return totalSecs;
    } else if ((total % 1440) == 0) {
        /* Intervals in whole days are also allowed */
        return totalSecs;
    } else {
        throw std::runtime_error(
                "Interval must be a number of days"
                "<n>d, or one of: 12h, 6h, 3h, 1h, 20m, 30m, 15m, 10m or 5m");
    }
}

std::string IntervalSchedule::intervalToString(unsigned interval)
{
    int seconds = interval % 60;
    interval /= 60;
    int minutes = interval % 60;
    interval /= 60;
    int hours = interval % 24;
    interval /= 24;

    std::stringstream intervalStr;
    if (interval) {
        intervalStr << interval << "d";
    }

    if (hours) {
        intervalStr << hours << "h";
    }

    if (minutes) {
        intervalStr << minutes << "m";
    }

    if (seconds) {
        intervalStr << seconds << "s";
    }

    return intervalStr.str();
}

PreciseIntervalSchedule::PreciseIntervalSchedule(std::shared_ptr<Activity> activity,
                                                 time_t start,
                                                 unsigned interval,
                                                 time_t end)
    : IntervalSchedule(activity, start, interval, end)
{
}

PreciseIntervalSchedule::~PreciseIntervalSchedule()
{
}

time_t PreciseIntervalSchedule::getBaseStartTime() const
{
    return m_start;
}

MojErr PreciseIntervalSchedule::toJson(MojObject& rep, unsigned long flags) const
{
    MojErr err = rep.putBool(_T("precise"), true);
    MojErrCheck(err);

    return IntervalSchedule::toJson(rep, flags);
}

RelativeIntervalSchedule::RelativeIntervalSchedule(std::shared_ptr<Activity> activity,
                                                   time_t start,
                                                   unsigned interval,
                                                   time_t end)
    : IntervalSchedule(activity, start, interval, end)
{
}

RelativeIntervalSchedule::~RelativeIntervalSchedule()
{
}

time_t RelativeIntervalSchedule::getBaseStartTime() const
{
    if (m_lastFinished != kNever) {
        return m_lastFinished;
    } else {
        return m_start;
    }
}

MojErr RelativeIntervalSchedule::toJson(MojObject& rep, unsigned long flags) const
{
    MojErr err;

    err = rep.putBool(_T("precise"), true);
    MojErrCheck(err);

    err = rep.putBool(_T("relative"), true);
    MojErrCheck(err);

    return IntervalSchedule::toJson(rep, flags);
}

