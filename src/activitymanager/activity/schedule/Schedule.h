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

#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include <boost/intrusive/set.hpp>

#include "Main.h"

class Activity;
class AbstractScheduleManager;

class Schedule: public std::enable_shared_from_this<Schedule> {
public:
    Schedule(std::shared_ptr<Activity> activity,
             time_t start);
    virtual ~Schedule();

    static const time_t kDayOne;
    static const time_t kNever;
    static const time_t kUnbounded;

    virtual time_t getNextStartTime() const;
    virtual void calcNextStartTime();

    time_t getTime() const;

    std::shared_ptr<Activity> getActivity() const;

    bool operator<(const Schedule& rhs) const;

    void queue();
    void unQueue();
    bool isQueued() const;

    void scheduled();
    bool isScheduled() const;

    /* INTERFACE:  ACTIVITY ----> SCHEDULE */

    /* Activity has finished running, and all subscribers have unsubscribed. */
    virtual void informActivityFinished();

    /* END INTERFACE */

    virtual bool shouldReschedule() const;

    void setLocal(bool local);
    bool isLocal() const;

    virtual bool isInterval() const;

    virtual MojErr toJson(MojObject& rep, unsigned long flags) const;


private:
    Schedule(const Schedule& copy);
    Schedule& operator=(const Schedule& copy);

protected:

    friend class AbstractScheduleManager;

    typedef boost::intrusive::set_member_hook<
            boost::intrusive::link_mode<boost::intrusive::auto_unlink>> QueueItem;

    static MojLogger s_log;

    QueueItem m_queueItem;

    std::weak_ptr<Activity> m_activity;

    time_t m_start;

    bool m_local;

    bool m_scheduled;
};

#endif /* __SCHEDULE_H__ */
