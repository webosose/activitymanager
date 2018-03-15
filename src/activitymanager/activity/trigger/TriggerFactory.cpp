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

#include <activity/trigger/ConcreteTrigger.h>
#include <activity/trigger/TriggerFactory.h>
#include <activity/trigger/TriggerSubscription.h>
#include "service/BusConnection.h"
#include "matcher/WhereMatcher.h"
#include "matcher/KeyMatcher.h"
#include "matcher/CompareMatcher.h"
#include "matcher/SimpleMatcher.h"

TriggerFactory::TriggerFactory()
{
}

TriggerFactory::~TriggerFactory()
{
}

std::shared_ptr<ITrigger> TriggerFactory::createKeyedTrigger(
        std::shared_ptr<Activity> activity,
        const LunaURL& url,
        const MojObject& params,
        const MojString& key)
{
    std::shared_ptr<Matcher> matcher = std::make_shared<KeyMatcher>(key);

    return createTrigger(activity, url, params, matcher);
}

std::shared_ptr<ITrigger> TriggerFactory::createBasicTrigger(
        std::shared_ptr<Activity> activity,
        const LunaURL& url,
        const MojObject& params)
{
    std::shared_ptr<Matcher> matcher = std::make_shared<SimpleMatcher>();

    return createTrigger(activity, url, params, matcher);
}

std::shared_ptr<ITrigger> TriggerFactory::createCompareTrigger(
        std::shared_ptr<Activity> activity,
        const LunaURL& url,
        const MojObject& params,
        const MojString& key,
        const MojObject& value)
{
    std::shared_ptr<Matcher> matcher = std::make_shared<CompareMatcher>(key, value);

    return createTrigger(activity, url, params, matcher);
}

std::shared_ptr<ITrigger> TriggerFactory::createWhereTrigger(
        std::shared_ptr<Activity> activity,
        const LunaURL& url,
        const MojObject& params,
        const MojObject& where)
{
    std::shared_ptr<Matcher> matcher = std::make_shared<WhereMatcher>(where);

    return createTrigger(activity, url, params, matcher);
}

std::shared_ptr<ITrigger> TriggerFactory::createTrigger(
        std::shared_ptr<Activity> activity,
        const LunaURL& url,
        const MojObject& params,
        std::shared_ptr<Matcher> matcher)
{
    std::shared_ptr<ConcreteTrigger> trigger =
            std::make_shared<ConcreteTrigger>(activity, matcher);

    std::shared_ptr<TriggerSubscription> subscription =
            std::make_shared<TriggerSubscriptionExclusive>(trigger, url, params);

    trigger->setSubscription(subscription);

    return trigger;
}

