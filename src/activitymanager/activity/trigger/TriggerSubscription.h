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

#ifndef __TRIGGER_SUBSCRIPTION_H__
#define __TRIGGER_SUBSCRIPTION_H__

#include <activity/trigger/ConcreteTrigger.h>
#include "Main.h"
#include "base/LunaCall.h"
#include "base/LunaURL.h"

class TriggerSubscription : public std::enable_shared_from_this<TriggerSubscription> {
public:
    TriggerSubscription(std::shared_ptr<ConcreteTrigger> trigger,
                        const LunaURL& url,
                        const MojObject& params);
    virtual ~TriggerSubscription();

    const LunaURL& getURL() const;

    virtual void subscribe();
    void unsubscribe();

    bool isSubscribed() const;

    MojErr toJson(MojObject& rep, unsigned flags) const;

    pbnjson::JValue toJson() const;

protected:
    void processResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);

    static MojLogger s_log;

    std::weak_ptr<ConcreteTrigger> m_trigger;

    LunaURL m_url;
    MojObject m_params;

    std::shared_ptr<LunaCall> m_call;
};

class TriggerSubscriptionExclusive : public TriggerSubscription {
public:
    TriggerSubscriptionExclusive(std::shared_ptr<ConcreteTrigger> trigger,
                                 const LunaURL& url,
                                 const MojObject& params);
    virtual ~TriggerSubscriptionExclusive();

    virtual void subscribe();
};

#endif /* __TRIGGER_SUBSCRIPTION_H__ */
