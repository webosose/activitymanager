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

#ifndef __ABSTRACT_POWER_ACTIVITY_H__
#define __ABSTRACT_POWER_ACTIVITY_H__

#include <boost/intrusive/list.hpp>

#include "Main.h"

class PowerManager;
class Activity;

class AbstractPowerActivity: public std::enable_shared_from_this<AbstractPowerActivity> {
public:
    enum PowerState {
        kPowerLocked,
        kPowerUnlocked,
        kPowerUnknown
    };

public:
    AbstractPowerActivity(std::shared_ptr<Activity> activity);
    virtual ~AbstractPowerActivity();

    virtual PowerState getPowerState() const = 0;

    virtual void begin() = 0;
    virtual void end() = 0;

    std::shared_ptr<Activity> getActivity();
    std::shared_ptr<const Activity> getActivity() const;

protected:
    friend class PowerManager;

    typedef boost::intrusive::list_member_hook<
            boost::intrusive::link_mode<boost::intrusive::auto_unlink>> ListItem;
    ListItem m_listItem;

    std::weak_ptr<Activity> m_activity;
};

class NoopPowerActivity: public AbstractPowerActivity {
public:
    NoopPowerActivity(std::shared_ptr<Activity> activity);
    virtual ~NoopPowerActivity();

    virtual PowerState getPowerState() const;

    virtual void begin();
    virtual void end();

protected:
    PowerState m_state;
};

#endif /* __ABSTRACT_POWER_ACTIVITY_H__ */
