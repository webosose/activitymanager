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

#ifndef __ABSTRACT_POWER_MANAGER_H__
#define __ABSTRACT_POWER_MANAGER_H__

#include "Main.h"
#include "AbstractPowerActivity.h"

/* PowerManager is responsible for the power activity management APIs */

class PowerManager {
public:
    static PowerManager& getInstance()
    {
        static PowerManager _instance;
        return _instance;
    }

    virtual ~PowerManager();

    virtual std::shared_ptr<AbstractPowerActivity> createPowerActivity(
            std::shared_ptr<Activity> activity);

    /* The Activity will request permission to begin or end, which will set
     * into motion a chain of events that will ultimately call its being
     * informed that power has been locked or unlocked, at which point it
     * will confirm to the Power Manager this is complete.
     *
     * Control flows from the Activity -> Power Manager -> Power Activity,
     * then from the Power Activity -> Activity -> Power Manager.
     */
    virtual void requestBeginPowerActivity(std::shared_ptr<Activity> activity);
    virtual void requestEndPowerActivity(std::shared_ptr<Activity> activity);
    virtual void confirmPowerActivityBegin(std::shared_ptr<Activity> activity);
    virtual void confirmPowerActivityEnd(std::shared_ptr<Activity> activity);

    MojErr infoToJson(MojObject& rep) const;

private:
    PowerManager();

    typedef boost::intrusive::member_hook<AbstractPowerActivity,
            AbstractPowerActivity::ListItem, &AbstractPowerActivity::m_listItem> PowerActivityListOption;
    typedef boost::intrusive::list<AbstractPowerActivity, PowerActivityListOption,
            boost::intrusive::constant_time_size<false>> PowerActivityList;

    PowerActivityList m_poweredActivities;
    unsigned long m_serial;
};

#endif /* __ABSTRACT_POWER_MANAGER_H__ */
