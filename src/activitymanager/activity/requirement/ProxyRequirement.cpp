// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "ProxyRequirement.h"
#include "util/Logging.h"

ProxyRequirement::ProxyRequirement(std::shared_ptr<IRequirement> req)
    : m_req(req)
    , m_isUserDefined(false)
    , m_isSatisfied(false)
{
}

ProxyRequirement::~ProxyRequirement()
{
}

const std::string& ProxyRequirement::getName() const
{
    return m_req->getName();
}

bool ProxyRequirement::isMet() const
{
    if (m_isUserDefined) {
        return m_isSatisfied;
    } else {
        return m_req->isMet();
    }
}

void ProxyRequirement::addListener(std::shared_ptr<IRequirementListener> listener)
{
    m_listener = listener;

    m_req->addListener(shared_from_this());
}

void ProxyRequirement::removeListener(std::shared_ptr<IRequirementListener> listener)
{
    m_listener.reset();

    m_req->removeListener(shared_from_this());
}

MojErr ProxyRequirement::toJson(MojObject& rep, unsigned flags) const
{
    MojErr err = MojErrNone;

    if (m_isUserDefined) {
        err = rep.put(getName().c_str(), isMet());
        MojErrCheck(err);
    } else {
        m_req->toJson(rep, flags);
    }

    return err;
}

pbnjson::JValue ProxyRequirement::toJson() const
{
    if (m_isUserDefined) {
        return pbnjson::JObject{
                { "name", getName() },
                { "satisfied", isMet() }
        };
    } else {
        return m_req->toJson();
    }
}

void ProxyRequirement::setSatisfied(bool satisfied)
{
    bool previous = isMet();
    m_isUserDefined = true;

    if (satisfied) {
        m_isSatisfied = true;
        if (!previous) {
            m_listener.lock()->onMetRequirement(getName());
        }
    } else {
        m_isSatisfied = false;
        if (previous) {
            m_listener.lock()->onUnmetRequirement(getName());
        }
    }
}

void ProxyRequirement::unsetSatisfied()
{
    m_isUserDefined = false;
}

void ProxyRequirement::onMetRequirement(std::string requirement)
{
    if (!m_isUserDefined) {
        m_listener.lock()->onMetRequirement(requirement);
    }
}

void ProxyRequirement::onUnmetRequirement(std::string requirement)
{
    if (!m_isUserDefined) {
        m_listener.lock()->onUnmetRequirement(requirement);
    }
}

void ProxyRequirement::onUpdateRequirement(std::string requirement)
{
    if (!m_isUserDefined) {
        m_listener.lock()->onUpdateRequirement(requirement);
    }
}
