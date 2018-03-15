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

#ifndef __REQUIREMENT_MANAGER_H__
#define __REQUIREMENT_MANAGER_H__

#include <map>

#include "Main.h"
#include "requirement/Requirement.h"
#include "base/IManager.h"
#include "base/IStringify.h"

class RequirementManager : public IManager, public IStringify {
public:
    static RequirementManager& getInstance()
    {
        static RequirementManager _instance;
        return _instance;
    }

    virtual ~RequirementManager();

    void initialize();
    std::shared_ptr<IRequirement> getRequirement(
            const std::string& name,
            const MojObject& value);

    virtual void enable();
    virtual void disable();

    MojErr infoToJson(MojObject& reply) const;
    virtual pbnjson::JValue toJson() const;

private:
    RequirementManager();

    typedef std::map<std::string, std::shared_ptr<Requirement>> RequirementMap;

    RequirementMap m_requirements;
};

#endif /* __REQUIREMENT_MANAGER_H__ */
