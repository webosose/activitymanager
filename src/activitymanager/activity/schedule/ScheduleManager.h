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

#ifndef __POWERD_SCHEDULER_H__
#define __POWERD_SCHEDULER_H__

#include <activity/schedule/AbstractScheduleManager.h>
#include "base/LunaCall.h"

class ScheduleManager: public AbstractScheduleManager {
public:
    static ScheduleManager& getInstance()
    {
        static ScheduleManager _instance;
        return _instance;
    }

    virtual ~ScheduleManager();

    void scheduledWakeup();

    virtual void enable();

private:
    ScheduleManager();
    static const char *kPowerdWakeupKey;

    virtual void updateTimeout(time_t nextWakeup, time_t curTime);
    virtual void cancelTimeout();

    void monitorSystemTime();

    size_t formatWakeupTime(time_t wake, char *at, size_t len) const;

    void handleTimeoutSetResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);
    void handleTimeoutClearResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);
    void handleSystemTimeResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);

    std::shared_ptr<LunaCall> m_call;
    std::shared_ptr<LunaCall> m_systemTimeCall;
};

#endif /* __POWERD_SCHEDULER_H__ */
