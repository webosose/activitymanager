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

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "Main.h"
#include "Schedule.h"

class AbstractScheduleManager {
public:
    AbstractScheduleManager();
    virtual ~AbstractScheduleManager();

    void addItem(std::shared_ptr<Schedule> item);
    void removeItem(std::shared_ptr<Schedule> item);

    static std::string timeToString(time_t convert, bool isUTC);
    static time_t stringToTime(const char *convert, bool& isUTC);

    void setLocalOffset(off_t offset);
    off_t getLocalOffset() const;

    time_t getSmartBaseTime() const;

    virtual void enable() = 0;

protected:
    virtual void updateTimeout(time_t nextWakeup, time_t curTime) = 0;
    virtual void cancelTimeout() = 0;

    typedef boost::intrusive::member_hook<Schedule, Schedule::QueueItem,
            &Schedule::m_queueItem> ScheduleQueueOption;
    typedef boost::intrusive::multiset<Schedule, ScheduleQueueOption,
            boost::intrusive::constant_time_size<false>> ScheduleQueue;

    void wake();
    static gboolean dequeueAndUpdateTimeout(gpointer data);
    void dequeueAndUpdateTimeout();
    void processQueue(ScheduleQueue& queue, time_t curTime);
    void reQueue(ScheduleQueue& queue);

    void timeChanged();

    time_t getNextStartTime() const;

    static MojLogger s_log;

    /* Priority queues of tasks, in order, sorted by next run time.
     * Two queues, one in absolute time, and one in local time. */
    ScheduleQueue m_queue;
    ScheduleQueue m_localQueue;

    time_t m_nextWakeup;
    bool m_wakeScheduled;

    bool m_localOffsetSet;
    off_t m_localOffset;

    time_t m_smartBase;
};

#endif /* _SCHEDULER_H_ */
