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

#include "Requirement.h"

#include "base/LunaCall.h"
#include "base/LunaURL.h"
#include "conf/ActivityJson.h"
#include "service/BusConnection.h"

const std::string& Requirement::getName() const
{
    return m_name;
}

MojErr Requirement::toJson(MojObject& rep, unsigned flags) const
{
    MojErr err = MojErrNone;

    if (flags & ACTIVITY_JSON_CURRENT) {
        if (m_current.type() != MojObject::TypeUndefined) {
            err = rep.put(m_name.c_str(), m_current);
            MojErrCheck(err);
        } else {
            err = rep.putBool(m_name.c_str(), isMet());
            MojErrCheck(err);
        }
    } else {
        err = rep.put(m_name.c_str(), m_value);
        MojErrCheck(err);
    }

    return err;
}

bool Requirement::setCurrentValue(const MojObject& current)
{
    if (m_current == current) {
        return false;
    }
    m_current = current;
    return true;
}

void Requirement::met()
{
    m_met = true;
}

void Requirement::unmet()
{
    m_met = false;
}

bool Requirement::isMet() const
{
    return m_met;
}

Requirement::Requirement(std::shared_ptr<RequirementInfo> info,
                         const std::string& name,
                         const MojObject& value,
                         bool met)
    : m_reqInfo(info)
    , m_name(name)
    , m_value(value)
    , m_met(met)
    , m_matcher(std::make_shared<WhereMatcher>(info->where))
{
}

Requirement::~Requirement()
{
}


pbnjson::JValue Requirement::toJson() const
{
    return pbnjson::JObject{
            { "name", getName() },
            { "satisfied", isMet() }
    };
}

void Requirement::addListener(std::shared_ptr<IRequirementListener> listener)
{
    m_listeners.push_back(listener);
}

void Requirement::removeListener(std::shared_ptr<IRequirementListener> listener)
{
    for (auto it = m_listeners.begin(); it != m_listeners.end(); ) {
        if ((*it).expired() || (*it).lock() == listener) {
            it = m_listeners.erase(it);
        } else {
            ++it;
        }
    }
}

void Requirement::subscribe()
{
    LOG_AM_DEBUG("Enabling Requirement %s", getName().c_str());

    LunaURL method(m_reqInfo->method.c_str());

    m_call = std::make_shared<LunaWeakPtrCall<Requirement>>(
            shared_from_this(),
            &Requirement::processResponse,
            true,
            method,
            m_reqInfo->params,
            LunaCall::kUnlimited);
    m_call->call();
}

void Requirement::unsubscribe()
{
    LOG_AM_DEBUG("Disabling Requirement %s", getName().c_str());

    m_call.reset();
}

void Requirement::processResponse(
        MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    if (err != MojErrNone) {
        bool subscribed = false;
        bool found = response.get(_T("subscribed"), subscribed);
        if (found && subscribed) {
            LOG_AM_WARNING(
                    MSGID_REQ_SUBSCR_WAIT, 1,
                    PMLOGKS("name", getName().c_str()),
                    "Subscription succeed, waiting: %s", MojoObjectJson(response).c_str());
        } else if (LunaCall::isPermanentFailure(msg, response, err)) {
            LOG_AM_WARNING(
                    MSGID_REQ_SUBSCR_FAIL, 1,
                    PMLOGKS("name", getName().c_str()),
                    "Subscription failed, cancelling: %s", MojoObjectJson(response).c_str());
            m_call.reset();
        } else {
            LOG_AM_WARNING(
                    MSGID_REQ_SUBSCR_RETRY, 1,
                    PMLOGKS("name", getName().c_str()),
                    "Subscription failed, resubscribing: %s", MojoObjectJson(response).c_str());
            static struct timespec sleep = { 0, 250000000 };
            nanosleep(&sleep, NULL);
            m_call->call();
        }
        return;
    }

    LOG_AM_DEBUG("Update from Requirement %s: %s", getName().c_str(),
                 MojoObjectJson(response).c_str());

    bool matched = m_matcher->match(response);
    bool updated = setCurrentValue(response);

    if (matched) {
        if (!isMet()) {
            LOG_AM_INFO(MSGID_REQ_SUBSCR, 1, PMLOGKS("name", getName().c_str()), "met");
            met();
            notifyToListeners(&IRequirementListener::onMetRequirement);
        } else if (updated) {
            LOG_AM_DEBUG("Requirement %s is updated", getName().c_str());
            notifyToListeners(&IRequirementListener::onUpdateRequirement);
        }
    } else {
        if (isMet()) {
            LOG_AM_INFO(MSGID_REQ_SUBSCR, 1, PMLOGKS("name", getName().c_str()), "unmet");
            unmet();
            notifyToListeners(&IRequirementListener::onUnmetRequirement);
        }
    }
}

void Requirement::notifyToListeners(std::function<void (IRequirementListener&, std::string)> func)
{
    for (auto it = m_listeners.begin(); it != m_listeners.end(); ) {
        std::shared_ptr<IRequirementListener> listener = (*it).lock();
        if (listener) {
            func(*listener, getName());
            ++it;
        } else {
            it = m_listeners.erase(it);
        }
    }
}

std::shared_ptr<RequirementInfo> Requirement::getRequirementInfo() const
{
    return m_reqInfo;
}

void Requirement::setSatisfied(bool satisfied)
{
    throw std::runtime_error("Not implemented");
}

void Requirement::unsetSatisfied()
{
    throw std::runtime_error("Not implemented");
}
