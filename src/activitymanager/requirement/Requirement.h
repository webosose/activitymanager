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

#ifndef __REQUIREMENT_H__
#define __REQUIREMENT_H__

#include <list>
#include <memory>

#include <core/MojString.h>
#include <core/MojErr.h>

#include "Main.h"
#include "activity/ActivityManager.h"
#include "activity/trigger/matcher/WhereMatcher.h"
#include "base/IRequirement.h"
#include "base/LunaCall.h"
#include "conf/Config.h"

class Requirement : public IRequirement, public std::enable_shared_from_this<Requirement> {
public:
    Requirement(std::shared_ptr<RequirementInfo> info,
                const std::string& name,
                const MojObject& value,
                bool met = false);
    virtual ~Requirement();

    /* IRequirement */
    virtual const std::string& getName() const;
    virtual bool isMet() const;
    virtual void addListener(std::shared_ptr<IRequirementListener> listener);
    virtual void removeListener(std::shared_ptr<IRequirementListener> listener);
    virtual MojErr toJson(MojObject& rep, unsigned flags) const;
    virtual pbnjson::JValue toJson() const;
    virtual void setSatisfied(bool satisfied);
    virtual void unsetSatisfied();

    void subscribe();
    void unsubscribe();

    std::shared_ptr<RequirementInfo> getRequirementInfo() const;

protected:
    bool setCurrentValue(const MojObject& current);

    void met();
    void unmet();

    void processResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);
    void notifyToListeners(std::function<void (IRequirementListener&, std::string)> func);

    std::shared_ptr<RequirementInfo> m_reqInfo;
    std::string m_name;
    MojObject m_value;
    MojObject m_current;

    bool m_met;
    std::shared_ptr<LunaCall> m_call;
    std::shared_ptr<WhereMatcher> m_matcher;

    std::list<std::weak_ptr<IRequirementListener>> m_listeners;
};

#endif /* __REQUIREMENT_H__ */
