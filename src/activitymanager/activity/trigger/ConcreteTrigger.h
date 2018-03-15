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

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#include <list>

#include "activity/Activity.h"
#include "base/ITrigger.h"
#include "base/IStringify.h"

class Matcher;
class TriggerSubscription;

class ConcreteTrigger : public ITrigger, public IStringify, public std::enable_shared_from_this<ConcreteTrigger> {
public:
    ConcreteTrigger(std::shared_ptr<Activity> activity, std::shared_ptr<Matcher> matcher);
    virtual ~ConcreteTrigger();

    void setSubscription(std::shared_ptr<TriggerSubscription> subscription);
    std::shared_ptr<TriggerSubscription> getSubscription() const;

    virtual void setName(const std::string& name);
    virtual const std::string& getName() const;

    virtual void init();
    virtual void clear();

    virtual bool isInit() const;
    virtual int getSubscriptionCount();

    virtual bool isSatisfied() const;

    virtual std::shared_ptr<Activity> getActivity();
    virtual std::shared_ptr<const Activity> getActivity() const;

    virtual void processResponse(const MojObject& response, MojErr err);

    virtual MojErr toJson(MojObject& rep, unsigned flags) const;
    virtual pbnjson::JValue toJson() const;

    virtual void setSatisfied(bool satisfied);
    virtual void unsetSatisfied();

private:
    void subscribe();
    void unsubscribe();

    std::weak_ptr<Activity> m_activity;

    std::shared_ptr<Matcher> m_matcher;
    std::string m_name;
    std::shared_ptr<TriggerSubscription> m_subscription;

    MojObject m_response;
    int m_subscriptionCount;
    bool m_isSatisfied;
    bool m_isUserDefined;
};

#endif /* __TRIGGER_H__ */
