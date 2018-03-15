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

#ifndef __SUBSCRIPTION_H__
#define __SUBSCRIPTION_H__

#include <core/MojService.h>
#include <core/MojServiceMessage.h>

#include "base/AbstractSubscription.h"
#include "base/BusId.h"
#include "base/Subscriber.h"

/* This wraps the libmojo/LS2 communications abstraction.  A MojoSubscription
 * will exist until cancelled from within the LS2 layer (the MojSignalHandler
 * maintains the reference, so live MojoSubscriptions can maintain the
 * reference counts of other objects - such as Activities)
 */

class Subscription: public AbstractSubscription {
public:
    Subscription(std::shared_ptr<Activity> activity, bool detailedEvents, MojServiceMessage *msg);
    virtual ~Subscription();

    virtual void enableSubscription();

    virtual MojErr sendEvent(ActivityEvent event);

    Subscriber& getSubscriber();
    const Subscriber& getSubscriber() const;

    static std::string getSubscriberString(MojServiceMessage *msg);
    static std::string getSubscriberString(MojRefCountedPtr<MojServiceMessage> msg);
    static BusId getBusId(MojServiceMessage *msg);
    static BusId getBusId(MojRefCountedPtr<MojServiceMessage> msg);
    static std::string getServiceName(MojRefCountedPtr<MojServiceMessage> msg);

    static void fixAppId(std::string& appId);

protected:
    virtual void handleCancel();

    class MojoCancelHandler: public MojSignalHandler {
    public:
        MojoCancelHandler(std::shared_ptr<AbstractSubscription> subscription,
                          MojServiceMessage *msg);
        virtual ~MojoCancelHandler();

    private:
        MojErr handleCancel(MojServiceMessage *msg);

        std::shared_ptr<AbstractSubscription> m_subscription;
        MojServiceMessage *m_msg;
        MojServiceMessage::CancelSignal::Slot<MojoCancelHandler> m_cancelSlot;
    };

    class MojoSubscriber: public Subscriber {
    public:
        MojoSubscriber(MojServiceMessage *msg);
        virtual ~MojoSubscriber();
    };

    MojoSubscriber m_subscriber;
    MojRefCountedPtr<MojServiceMessage> m_msg;
};

#endif /* __SUBSCRIPTION_H__ */
