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

#include <activity/trigger/TriggerSubscription.h>
#include "util/MojoObjectJson.h"
#include "service/BusConnection.h"

TriggerSubscription::TriggerSubscription(
        std::shared_ptr<ConcreteTrigger> trigger,
        const LunaURL& url,
        const MojObject& params)
    : m_trigger(trigger)
    , m_url(url)
    , m_params(params)
{
}

TriggerSubscription::~TriggerSubscription()
{
}

const LunaURL& TriggerSubscription::getURL() const
{
    return m_url;
}

void TriggerSubscription::subscribe()
{
    if (m_call) {
        return;
    }

    m_call = std::make_shared<LunaWeakPtrCall<TriggerSubscription>>(
            shared_from_this(),
            &TriggerSubscription::processResponse,
            false,
            m_url,
            m_params,
            LunaCall::kUnlimited);
    m_call->call();
}

void TriggerSubscription::unsubscribe()
{
    if (m_call) {
        m_call.reset();
    }
}

bool TriggerSubscription::isSubscribed() const
{
    return m_call != NULL;
}

void TriggerSubscription::processResponse(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    m_trigger.lock()->processResponse(response, err);
}

MojErr TriggerSubscription::toJson(MojObject& rep, unsigned flags) const
{
    MojErr err;

    err = rep.putString(_T("method"), m_url.getString().c_str());
    MojErrCheck(err);

    if (m_params.type() != MojObject::TypeUndefined) {
        err = rep.put(_T("params"), m_params);
        MojErrCheck(err);
    }

    return MojErrNone;
}

pbnjson::JValue TriggerSubscription::toJson() const
{
    pbnjson::JValue params = pbnjson::JDomParser::fromString(MojoObjectJson(m_params).c_str());

    return pbnjson::JObject{
            { "method", m_url.getString() },
            { "params", params }
    };
}

TriggerSubscriptionExclusive::TriggerSubscriptionExclusive(
        std::shared_ptr<ConcreteTrigger> trigger,
        const LunaURL& url,
        const MojObject& params)
    : TriggerSubscription(trigger, url, params)
{
}

TriggerSubscriptionExclusive::~TriggerSubscriptionExclusive()
{
}

void TriggerSubscriptionExclusive::subscribe()
{
    if (m_call) {
        return;
    }
    m_call = std::make_shared<LunaWeakPtrCall<TriggerSubscription>>(
            shared_from_this(),
            &TriggerSubscriptionExclusive::processResponse,
            false,
            m_url,
            m_params,
            LunaCall::kUnlimited);
    if (m_call != nullptr) {
        m_call->call(std::dynamic_pointer_cast<ConcreteTrigger, ConcreteTrigger>(
                m_trigger.lock())->getActivity());
    }
}

