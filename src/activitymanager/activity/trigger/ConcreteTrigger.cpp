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
#include <activity/trigger/TriggerSubscription.h>
#include <stdexcept>

#include "Matcher.h"
#include "activity/Activity.h"
#include "conf/ActivityJson.h"
#include "util/Logging.h"

ConcreteTrigger::ConcreteTrigger(std::shared_ptr<Activity> activity, std::shared_ptr<Matcher> matcher)
    : m_activity(activity)
    , m_matcher(matcher)
    , m_subscriptionCount(0)
    , m_isSatisfied(false)
    , m_isUserDefined(false)
{
}

ConcreteTrigger::~ConcreteTrigger()
{
}

void ConcreteTrigger::setSubscription(std::shared_ptr<TriggerSubscription> subscription)
{
    m_subscription = subscription;
}

std::shared_ptr<TriggerSubscription> ConcreteTrigger::getSubscription() const
{
    return m_subscription;
}

void ConcreteTrigger::setName(const std::string& name)
{
    m_name = name;
}

const std::string& ConcreteTrigger::getName() const
{
    return m_name;
}

void ConcreteTrigger::init()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (!m_subscription) {
        LOG_AM_DEBUG("[Activity %llu] Can't arm trigger that has no subscription",
                     m_activity.lock()->getId()); // LATHA_TBD
        throw std::runtime_error("Can't arm trigger that has no subscription");
    }

    LOG_AM_DEBUG("[Activity %llu] Arming Trigger on \"%s\"",
                 m_activity.lock()->getId(),
                 m_subscription->getURL().getString().c_str());

    m_isSatisfied = false;
    subscribe();
}

void ConcreteTrigger::subscribe()
{
    if (m_subscription && !m_subscription->isSubscribed()) {
        m_subscription->subscribe();
    }
}

void ConcreteTrigger::unsubscribe()
{
    if (m_subscription && m_subscription->isSubscribed()) {
        m_subscription->unsubscribe();
    }
}

void ConcreteTrigger::clear()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (m_subscription) {
        LOG_AM_DEBUG("[Activity %llu] Disarming Trigger on \"%s\"",
                     m_activity.lock()->getId(),
                     m_subscription->getURL().getString().c_str());
    } else {
        LOG_AM_DEBUG("[Activity %llu] Disarming Trigger", m_activity.lock()->getId());
    }

    m_isSatisfied = false;
    unsubscribe();
}

int ConcreteTrigger::getSubscriptionCount()
{
    return m_subscriptionCount;
}

bool ConcreteTrigger::isInit() const
{
    return m_subscription->isSubscribed();
}

bool ConcreteTrigger::isSatisfied() const
{
    return m_isSatisfied;
}

std::shared_ptr<Activity> ConcreteTrigger::getActivity()
{
    return m_activity.lock();
}

std::shared_ptr<const Activity> ConcreteTrigger::getActivity() const
{
    return m_activity.lock();
}

void ConcreteTrigger::processResponse(const MojObject& response, MojErr err)
{
    if (m_isUserDefined) {
        return;
    }

    MojObject subscribed;
    bool statusChanged = false;
    bool valueChanged = false;

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (m_response != response)
        valueChanged = true;
    m_subscriptionCount++;
    m_response = response;

    /* Subscription guarantees any errors received are from the subscribing
     * Service.  Transient bus errors are handled automatically.
     *
     * XXX have an option to disable auto-resubscribe */
    if (err) {
        if (response.get(_T("subscribed"), subscribed) &&
            subscribed.type() == MojObject::TypeBool && subscribed.boolValue()) {
            LOG_AM_DEBUG("[Activity %llu] Trigger call \"%s\" failed, but subscription is still available.",
                         m_activity.lock()->getId(),
                         m_subscription->getURL().getString().c_str());
            // This case may be considered a trigger failure,
            // but activitymanager has not considered this as a failure. So keep this policy.
            if (m_isSatisfied) {
                m_isSatisfied = false;
                statusChanged = true;
            }
            m_activity.lock()->onSuccessTrigger(shared_from_this(), statusChanged, valueChanged);
        } else {
            LOG_AM_WARNING("TRIGGER_FAIL", 4,
                           PMLOGKFV("Activity", "%llu", m_activity.lock()->getId()),
                           PMLOGKS("trigger", m_name.c_str()),
                           PMLOGKS("url", m_subscription->getURL().getString().c_str()),
                           PMLOGJSON("Response", MojoObjectJson(response).c_str()), "");
            m_activity.lock()->onFailTrigger(shared_from_this());
        }
        return;
    }

    if (m_matcher->match(response)) {
        LOG_AM_DEBUG("[Activity %llu] Trigger call \"%s\" fired!",
                     m_activity.lock()->getId(),
                     m_subscription->getURL().getString().c_str());
        if (!m_isSatisfied) {
            m_isSatisfied = true;
            statusChanged = true;
        }
    } else {
        LOG_AM_DEBUG("[Activity %llu] Trigger call \"%s\" is not triggered",
                     m_activity.lock()->getId(),
                     m_subscription->getURL().getString().c_str());
        if (m_isSatisfied) {
            m_isSatisfied = false;
            statusChanged = true;
        }
    }

    m_activity.lock()->onSuccessTrigger(shared_from_this(), statusChanged, valueChanged);
}

MojErr ConcreteTrigger::toJson(MojObject& rep, unsigned flags) const
{
    if (flags & ACTIVITY_JSON_CURRENT) {
        if (isSatisfied()) {
            rep = m_response;
        } else {
            rep = MojObject(false);
        }

        return MojErrNone;
    }

    MojErr err;

    err = m_matcher->toJson(rep, flags);
    MojErrCheck(err);

    if (m_subscription) {
        err = m_subscription->toJson(rep, flags);
        MojErrCheck(err);
    }

    return MojErrNone;
}

pbnjson::JValue ConcreteTrigger::toJson() const
{
    pbnjson::JValue root = m_subscription->toJson();
    root.put("name", m_name);
    root.put("satisfied", m_isSatisfied);

    return root;
}

void ConcreteTrigger::setSatisfied(bool satisfied)
{
    m_isUserDefined = true;

    m_subscriptionCount++;
    m_response = MojObject(satisfied);
    bool valueChanged = false;

    if (satisfied) {
        if (!m_isSatisfied) {
            m_isSatisfied = true;
            valueChanged = true;
        }
    } else {
        if (m_isSatisfied) {
            m_isSatisfied = false;
            valueChanged = true;
        }
    }

    m_activity.lock()->onSuccessTrigger(shared_from_this(), valueChanged, valueChanged);
}

void ConcreteTrigger::unsetSatisfied()
{
    m_isUserDefined = false;
}
