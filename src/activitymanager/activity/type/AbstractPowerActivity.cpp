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

#include "AbstractPowerActivity.h"

#include "../Activity.h"
#include "util/Logging.h"

AbstractPowerActivity::AbstractPowerActivity(std::shared_ptr<Activity> activity)
    : m_activity(activity)
{
}

AbstractPowerActivity::~AbstractPowerActivity()
{
}

std::shared_ptr<Activity> AbstractPowerActivity::getActivity()
{
    return m_activity.lock();
}

std::shared_ptr<const Activity> AbstractPowerActivity::getActivity() const
{
    return m_activity.lock();
}

NoopPowerActivity::NoopPowerActivity(std::shared_ptr<Activity> activity)
    : AbstractPowerActivity(activity), m_state(kPowerUnlocked)
{
}

NoopPowerActivity::~NoopPowerActivity()
{
}

AbstractPowerActivity::PowerState NoopPowerActivity::getPowerState() const
{
    return m_state;
}

void NoopPowerActivity::begin()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    auto activity_ptr = m_activity.lock();
    if (!activity_ptr) {
        return;
    }
    LOG_AM_DEBUG("[Activity %llu] Locking power on", activity_ptr->getId());

    if (m_state != kPowerLocked) {
        m_state = kPowerLocked;
        activity_ptr->powerLockedNotification();
    }
}

void NoopPowerActivity::end()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    auto activity_ptr = m_activity.lock();
    if (!activity_ptr) {
        return;
    }
    LOG_AM_DEBUG("[Activity %llu] Unlocking power", activity_ptr->getId());

    if (m_state != kPowerUnlocked) {
        m_state = kPowerUnlocked;
        activity_ptr->powerUnlockedNotification();
    }
}

