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

#include "AbstractSubscription.h"

#include <stdexcept>

#include "activity/Activity.h"
#include "util/Logging.h"

AbstractSubscription::AbstractSubscription(std::shared_ptr<Activity> activity, bool detailedUpdates)
    : m_detailedEvents(detailedUpdates)
    , m_plugged(false)
    , m_activity(activity)
{
}

AbstractSubscription::~AbstractSubscription()
{
}

void AbstractSubscription::handleCancelWrapper()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("%s cancelling subscription", getSubscriber().getString().c_str());

    try {
        if (!m_activity.expired()) {
            m_activity.lock()->removeSubscription(shared_from_this());
        }

        /* Any specialized implementation for the specific Subscription type */
        handleCancel();
    } catch (const std::exception& except) {
        LOG_AM_ERROR(MSGID_SUBSCRIPTION_CANCEL_FAIL, 1,
                     PMLOGKS("SUBSCRIBER",getSubscriber().getString().c_str() ),
                     "Unhandled exception \"%s\" occurred while cancelling subscription",
                     except.what());
    } catch (...) {
        LOG_AM_ERROR(MSGID_SUBSCRIPTION_CANCEL_ERR,1,
                     PMLOGKS("SUBSCRIBER",getSubscriber().getString().c_str() ),
                     "Unhandled exception of unknown type occurred cancelling subscription");
    }
}

bool AbstractSubscription::operator<(const AbstractSubscription& rhs) const
{
    return (this < &rhs);
}

MojErr AbstractSubscription::queueEvent(ActivityEvent event)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    /* Non-detailed subscriptions do not get update events */
    if ((event == kActivityUpdateEvent) && !m_detailedEvents) {
        return MojErrNone;
    }

    if (!isPlugged()) {
        return sendEvent(event);
    }

    /* If there's already an event queued, suppress this one.
     * Current Activity details will be pulled when the event is
     * *sent* */
    if ((event == kActivityUpdateEvent) && !m_eventQueue.empty()) {
        return MojErrNone;
    }

    m_eventQueue.push_back(event);
    return MojErrNone;
}

void AbstractSubscription::plug()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    m_plugged = true;
}

void AbstractSubscription::unplug()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    m_plugged = false;

    /* Don't issue any events if there are persist commands waiting */
    if (!m_activity.expired() && m_activity.lock()->isPersistCommandHooked()) {
        return;
    }

    while (!m_eventQueue.empty()) {
        ActivityEvent event = m_eventQueue.front();
        m_eventQueue.pop_front();
        sendEvent(event);
    }
}

bool AbstractSubscription::isPlugged() const
{
    return (m_plugged || (!m_activity.expired() && m_activity.lock()->isPersistCommandHooked()));
}

