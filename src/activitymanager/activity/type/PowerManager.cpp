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

#include <activity/type/PowerManager.h>
#include <stdexcept>
#include <algorithm>

#include "AbstractPowerActivity.h"
#include "activity/Activity.h"
#include "util/Logging.h"
#include "conf/Setting.h"
#include "PowerActivity.h"

PowerManager::PowerManager()
    : m_serial(0)
{
}

PowerManager::~PowerManager()
{
}

void PowerManager::requestBeginPowerActivity(std::shared_ptr<Activity> activity)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Activity %lu requesting to begin power activity", (unsigned long )activity->getId());

    std::shared_ptr<AbstractPowerActivity> powerActivity = activity->getPowerActivity();

    if (powerActivity->getPowerState() != AbstractPowerActivity::kPowerLocked) {
        m_poweredActivities.push_back(*powerActivity);
        powerActivity->begin();
    } else {
        activity->powerLockedNotification();
    }
}

void PowerManager::confirmPowerActivityBegin(std::shared_ptr<Activity> activity)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Activity %lu confirmed power activity running",
                 (unsigned long )activity->getId());
}

void PowerManager::requestEndPowerActivity(std::shared_ptr<Activity> activity)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Activity %lu request to end power activity",
                 (unsigned long )activity->getId());

    std::shared_ptr<AbstractPowerActivity> powerActivity = activity->getPowerActivity();

    if (powerActivity->getPowerState() != AbstractPowerActivity::kPowerUnlocked) {
        powerActivity->end();
    } else {
        activity->powerUnlockedNotification();
    }
}

void PowerManager::confirmPowerActivityEnd(std::shared_ptr<Activity> activity)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Activity %lu confirmed power activity ended",
                 (unsigned long )activity->getId());

    std::shared_ptr<AbstractPowerActivity> powerActivity = activity->getPowerActivity();

    if (powerActivity->m_listItem.is_linked()) {
        powerActivity->m_listItem.unlink();
    }
}

MojErr PowerManager::infoToJson(MojObject& rep) const
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;

    if (!m_poweredActivities.empty()) {
        MojObject poweredActivities(MojObject::TypeArray);

        std::shared_ptr<const Activity> (AbstractPowerActivity::*getActivityPtr)() const = &AbstractPowerActivity::getActivity;
        std::for_each(
                m_poweredActivities.begin(),
                m_poweredActivities.end(),
                boost::bind(&Activity::pushIdentityJson,
                            boost::bind(getActivityPtr, _1),
                            std::ref(poweredActivities)));

        err = rep.put(_T("poweredActivities"), poweredActivities);
        MojErrCheck(err);
    }

    return MojErrNone;
}

std::shared_ptr<AbstractPowerActivity> PowerManager::createPowerActivity(std::shared_ptr<Activity> activity)
{
    if (Setting::getInstance().isSimulator()) {
        return std::make_shared<NoopPowerActivity>(activity);
    } else {
        return std::make_shared<PowerActivity>(activity, m_serial++);
    }
}

