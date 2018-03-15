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

#ifndef __ABSTRACT_SUBSCRIPTION_H__
#define __ABSTRACT_SUBSCRIPTION_H__

#include <list>
#include <boost/intrusive/set.hpp>

#include "Main.h"
#include "base/Subscriber.h"
#include "conf/ActivityTypes.h"

class Activity;

class AbstractSubscription : public std::enable_shared_from_this<AbstractSubscription> {
public:
    AbstractSubscription(std::shared_ptr<Activity> activity, bool detailedEvents);
    virtual ~AbstractSubscription();

    virtual void enableSubscription() = 0;

    virtual Subscriber& getSubscriber() = 0;
    virtual const Subscriber& getSubscriber() const = 0;

    virtual MojErr sendEvent(ActivityEvent event) = 0;

    MojErr queueEvent(ActivityEvent event);

    void plug();
    void unplug();
    bool isPlugged() const;

    void handleCancelWrapper();

    bool operator<(const AbstractSubscription& rhs) const;

private:
    bool operator==(const AbstractSubscription& rhs) const;

protected:
    virtual void handleCancel() = 0;

    friend class Activity;

    typedef boost::intrusive::set_member_hook<boost::intrusive::link_mode<
            boost::intrusive::auto_unlink> > SetItem;
    SetItem m_item;

    bool m_detailedEvents;
    bool m_plugged;
    std::list<ActivityEvent> m_eventQueue;

    std::weak_ptr<Activity> m_activity;
};

#endif /* __ABSTRACT_SUBSCRIPTION_H__ */
