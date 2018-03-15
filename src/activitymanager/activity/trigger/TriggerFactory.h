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

#ifndef __TRIGGER_MANAGER_H__
#define __TRIGGER_MANAGER_H__

#include "Main.h"
#include "activity/Activity.h"
#include "activity/trigger/Matcher.h"
#include "base/ITrigger.h"
#include "base/LunaURL.h"

class TriggerFactory {
public:
    virtual ~TriggerFactory();

    static std::shared_ptr<ITrigger> createKeyedTrigger(
            std::shared_ptr<Activity> activity,
            const LunaURL& url,
            const MojObject& params,
            const MojString& key);

    static std::shared_ptr<ITrigger> createBasicTrigger(
            std::shared_ptr<Activity> activity,
            const LunaURL& url,
            const MojObject& params);

    static std::shared_ptr<ITrigger> createCompareTrigger(
            std::shared_ptr<Activity> activity,
            const LunaURL& url,
            const MojObject& params,
            const MojString& key,
            const MojObject& value);

    static std::shared_ptr<ITrigger> createWhereTrigger(
            std::shared_ptr<Activity> activity,
            const LunaURL& url,
            const MojObject& params,
            const MojObject& where);

private:
    TriggerFactory();

    static std::shared_ptr<ITrigger> createTrigger(
            std::shared_ptr<Activity> activity,
            const LunaURL& url,
            const MojObject& params,
            std::shared_ptr<Matcher> matcher);
};

#endif /* __TRIGGER_MANAGER_H__ */
