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

#include "Subscription.h"

#include <stdexcept>
#include <luna/MojLunaMessage.h>

#include "activity/Activity.h"
#include "util/Logging.h"

Subscription::Subscription(std::shared_ptr<Activity> activity,
                           bool detailedEvents,
                           MojServiceMessage *msg)
    : AbstractSubscription(activity, detailedEvents)
    , m_subscriber(msg)
    , m_msg(msg)
{
}

Subscription::~Subscription()
{
}

void Subscription::enableSubscription()
{
    if (m_activity.expired()) {
        return;
    }

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Enabling subscription for %s",
                 m_activity.lock()->getId(), getSubscriber().getString().c_str());

    /* The message will take a reference to to Cancel Handler when it
     * registers itself with the message. MojRefCountedPtr's reference
     * count is internal to the object pointed to. */
    MojRefCountedPtr<Subscription::MojoCancelHandler> cancelHandler(
            new Subscription::MojoCancelHandler(shared_from_this(), m_msg.get()));
}

Subscriber& Subscription::getSubscriber()
{
    return m_subscriber;
}

const Subscriber& Subscription::getSubscriber() const
{
    return m_subscriber;
}

MojErr Subscription::sendEvent(ActivityEvent event)
{
    if (!m_msg.get()) {
        return MojErrInvalidArg;
    }

    if (m_activity.expired()) {
        return MojErrInternal;
    }

    std::shared_ptr<Activity> activity = m_activity.lock();

    MojErr err = MojErrNone;
    MojObject eventReply;

    err = eventReply.putString(_T("event"), ActivityEventNames[event]);
    MojErrCheck(err);

    err = eventReply.putInt(_T("activityId"), activity->getId());
    MojErrCheck(err);

    err = eventReply.putBool(_T("subscribed"), true);
    MojErrCheck(err);

    err = eventReply.putBool(MojServiceMessage::ReturnValueKey, true);
    MojErrCheck(err);

    if (m_detailedEvents) {
        MojObject details(MojObject::TypeObject);

        err = activity->activityInfoToJson(details);
        MojErrCheck(err);

        err = eventReply.put(_T("$activity"), details);
        MojErrCheck(err);
    }

    err = m_msg->reply(eventReply);
    MojErrCheck(err);

    return MojErrNone;
}

std::string Subscription::getSubscriberString(MojServiceMessage *msg)
{
    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
    if (!lunaMsg) {
        throw std::runtime_error("Can't generate subscriber string from non-Luna message");
    }

    const char *appId = lunaMsg->appId();
    if (!appId) {
        const char *serviceId = lunaMsg->senderId();
        if (serviceId) {
            return BusId::getString(serviceId, BusService);
        } else {
            return BusId::getString(lunaMsg->senderAddress(), BusAnon);
        }
    }

    std::string appIdFixed(appId);
    fixAppId(appIdFixed);
    return BusId::getString(appIdFixed, BusApp);
}

std::string Subscription::getSubscriberString(MojRefCountedPtr<MojServiceMessage> msg)
{
    return getSubscriberString(msg.get());
}

BusId Subscription::getBusId(MojServiceMessage *msg)
{
    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
    if (!lunaMsg) {
        throw std::runtime_error("Can't generate subscriber string from non-Luna message");
    }

    const char *appId = lunaMsg->appId();
    if (!appId) {
        const char *serviceId = lunaMsg->senderId();
        if (serviceId) {
            return BusId(serviceId, BusService);
        } else {
            return BusId(lunaMsg->senderAddress(), BusAnon);
        }
    }

    std::string appIdFixed(appId);
    fixAppId(appIdFixed);
    return BusId(appIdFixed, BusApp);
}

BusId Subscription::getBusId(MojRefCountedPtr<MojServiceMessage> msg)
{
    return getBusId(msg.get());
}

std::string Subscription::getServiceName(MojRefCountedPtr<MojServiceMessage> msg)
{
    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg.get());
    if (!lunaMsg) {
        throw std::runtime_error("Can't generate subscriber string from non-Luna message");
    }

    const char *serviceId = lunaMsg->senderId();
    if (serviceId) {
        return serviceId;
    }

    return lunaMsg->senderAddress();
}

void Subscription::fixAppId(std::string& appId)
{
    size_t firstSpace = appId.find_first_of(' ');
    if (firstSpace != std::string::npos) {
        appId.resize(firstSpace);
    }
}

void Subscription::handleCancel()
{
    m_msg.reset();
}

Subscription::MojoCancelHandler::MojoCancelHandler(
            std::shared_ptr<AbstractSubscription> subscription, MojServiceMessage *msg)
    : m_subscription(subscription)
    , m_msg(msg)
    , m_cancelSlot(this, &Subscription::MojoCancelHandler::handleCancel)
{
    msg->notifyCancel(m_cancelSlot);
}

Subscription::MojoCancelHandler::~MojoCancelHandler()
{
}

MojErr Subscription::MojoCancelHandler::handleCancel(MojServiceMessage *msg)
{
    if (m_msg == msg) {
        m_subscription->handleCancelWrapper();
        return MojErrNone;
    } else {
        return MojErrInvalidArg;
    }
}

Subscription::MojoSubscriber::MojoSubscriber(MojServiceMessage *msg)
{
    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
    if (!lunaMsg) {
        throw std::runtime_error("Can't generate subscriber from non-Luna message");
    }

    const char *appId = lunaMsg->appId();
    if (!appId) {
        const char *serviceId = lunaMsg->senderId();
        if (serviceId) {
            m_id = BusId(serviceId, BusService);
        } else {
            m_id = BusId(lunaMsg->senderAddress(), BusAnon);
        }
        return;
    }

    std::string appIdFixed(appId);
    Subscription::fixAppId(appIdFixed);
    m_id = BusId(appIdFixed, BusApp);
}

Subscription::MojoSubscriber::~MojoSubscriber()
{
}

