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

#ifndef __POWER_ACTIVITY_H__
#define __POWER_ACTIVITY_H__

#include <activity/type/PowerManager.h>
#include "AbstractPowerActivity.h"
#include "base/LunaCall.h"
#include "base/Timeout.h"

class PowerActivity: public AbstractPowerActivity {
public:
    PowerActivity(std::shared_ptr<Activity> activity,
                  unsigned long serial);
    virtual ~PowerActivity();

    virtual PowerState getPowerState() const;

    virtual void begin();
    virtual void end();

    unsigned long getSerial() const;

protected:
    void powerLockedNotification(MojServiceMessage *msg, const MojObject& response, MojErr err);
    void powerUnlockedNotification(MojServiceMessage *msg, const MojObject& response, MojErr err, bool debounce);
    void powerUnlockedNotificationNormal(MojServiceMessage *msg, const MojObject& response, MojErr err);
    void powerUnlockedNotificationDebounce(MojServiceMessage *msg, const MojObject& response, MojErr err);

    void timeoutNotification();

    std::string getRemotePowerActivityName() const;

    MojErr createRemotePowerActivity(bool debounce = false);
    MojErr removeRemotePowerActivity();

    static const int kPowerActivityLockDuration; /* 300 */
    static const int kPowerActivityLockUpdateInterval; /* 240 */
    static const int kPowerActivityDebounceDuration; /*  12 */

    unsigned long m_serial;
    PowerState m_currentState;
    PowerState m_targetState;

    std::shared_ptr<LunaCall> m_call;
    std::shared_ptr<Timeout<PowerActivity>> m_timeout;
};

#endif /* __POWER_ACTIVITY_H__ */
