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

#include "RequirementManager.h"

#include <algorithm>
#include <stdexcept>

#include "conf/Config.h"
#include "util/Logging.h"
#include "service/BusConnection.h"

RequirementManager::RequirementManager()
{
}

RequirementManager::~RequirementManager()
{
}

void RequirementManager::initialize()
{
    auto requirements = Config::getInstance().getRequirements();

    for (auto it = requirements.begin(); it != requirements.end(); ++it) {
        std::shared_ptr<Requirement> req =
                std::make_shared<Requirement>((*it), (*it)->name, true);

        m_requirements[(*it)->name] = req;

        LOG_AM_INFO(MSGID_REQ_REGIST, 1, PMLOGKS("name", (*it)->name.c_str()), "registered");
    }
}

std::shared_ptr<IRequirement> RequirementManager::getRequirement(
        const std::string& name,
        const MojObject& value)
{
    // TODO move to Requirement
    if (value.type() != MojObject::TypeBool || !value.boolValue()) {
        throw std::runtime_error("If '" + name + "' requirement is specified, "
                                 "the only legal value is 'true'");
    }

    auto found = m_requirements.find(name);
    if (found == m_requirements.end()) {
        LOG_AM_ERROR(MSGID_REQ_UNKNOWN, 1,
                     PMLOGKS("Requirement", name.c_str()),
                     "Manager not found for instantiate ");
        throw std::runtime_error("Manager for requirement not found");
    }

    return found->second;
}

void RequirementManager::enable()
{
    std::for_each(m_requirements.begin(),
                  m_requirements.end(),
                  [] (RequirementMap::value_type& val) { val.second->subscribe(); });
}

void RequirementManager::disable()
{
    std::for_each(m_requirements.begin(),
                  m_requirements.end(),
                  [] (RequirementMap::value_type& val) { val.second->unsubscribe(); });
}

MojErr RequirementManager::infoToJson(MojObject& reply) const
{
    MojErr err = MojErrNone;

    MojObject reqsObj(MojObject::TypeArray);

    for (auto it = m_requirements.begin(); it != m_requirements.end(); ++it) {
        std::shared_ptr<Requirement> requirement = (*it).second;
        std::shared_ptr<RequirementInfo> info = requirement->getRequirementInfo();

        MojObject reqObj;

        err = info->toJson(reqObj);
        MojErrCheck(err);

        err = reqsObj.push(reqObj);
        MojErrCheck(err);
    }

    err = reply.put("requirements", reqsObj);
    MojErrCheck(err);

    return MojErrNone;
}

pbnjson::JValue RequirementManager::toJson() const
{
    JValue requirements = pbnjson::Array();
    for (auto it = m_requirements.begin(); it != m_requirements.end(); ++it) {
        std::shared_ptr<Requirement> requirement = (*it).second;
        std::shared_ptr<RequirementInfo> info = requirement->getRequirementInfo();

        requirements.append(info->toJson());
    }
    return requirements;
}
