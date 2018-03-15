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

#ifndef __INTERVAL_SCHEDULE_H__
#define __INTERVAL_SCHEDULE_H__

#include "Schedule.h"

/*
 * Interval Schedule items have a defined start time, and potentially an
 * end time.
 *
 * "interval" specifies how often (in seconds) the Activity should run.
 *
 * "smart" specifies that the the Schedule should be scheduled with other
 * items on a similar interval.  In order to ensure they align, only
 * particular intervals will be supported.  The interval will be rounded
 * *UP* to the nearest interval.  Initially supported intervals:
 * N days, 12 hours, 6 hours, 2 hours, 1 hour, 30 minutes, 15 minutes,
 * 10 minutes, 5 minutes.
 *
 */

class IntervalSchedule: public Schedule {
public:
    IntervalSchedule(std::shared_ptr<Activity> activity,
                     time_t start,
                     unsigned interval,
                     time_t end);
    virtual ~IntervalSchedule();

    virtual time_t getNextStartTime() const;
    virtual void calcNextStartTime();

    virtual time_t getBaseStartTime() const;

    virtual bool isInterval() const;

    void setSkip(bool skip);

    void setLastFinishedTime(time_t finished);

    /* INTERFACE:  ACTIVITY ----> SCHEDULE */

    virtual void informActivityFinished();

    /* END INTERFACE */

    virtual bool shouldReschedule() const;

    virtual MojErr toJson(MojObject& rep, unsigned long flags) const;

    static unsigned stringToInterval(const char *intervalStr, bool smart = false);
    static std::string intervalToString(unsigned interval);

protected:
    time_t m_end;

    unsigned m_interval;

    bool m_skip;

    time_t m_nextStart;
    time_t m_lastFinished;
};

class PreciseIntervalSchedule: public IntervalSchedule {
public:
    PreciseIntervalSchedule(std::shared_ptr<Activity> activity,
                            time_t start,
                            unsigned interval,
                            time_t end);
    virtual ~PreciseIntervalSchedule();

    virtual time_t getBaseStartTime() const;

    virtual MojErr toJson(MojObject& rep, unsigned long flags) const;
};

class RelativeIntervalSchedule: public IntervalSchedule {
public:
    RelativeIntervalSchedule(std::shared_ptr<Activity> activity,
                             time_t start,
                             unsigned interval,
                             time_t end);
    virtual ~RelativeIntervalSchedule();

    virtual time_t getBaseStartTime() const;

    virtual MojErr toJson(MojObject& rep, unsigned long flags) const;
};

#endif /* __INTERVAL_SCHEDULE_H__ */
