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

#ifndef __PROXY_REQUIREMENT_H__
#define __PROXY_REQUIREMENT_H__

#include <core/MojString.h>
#include <core/MojErr.h>

#include "Main.h"
#include "base/IRequirement.h"

class ProxyRequirement : public IRequirement, public IRequirementListener,
                         public std::enable_shared_from_this<ProxyRequirement> {
public:
    ProxyRequirement(std::shared_ptr<IRequirement> req);
    virtual ~ProxyRequirement();

    // IRequirement
    virtual const std::string& getName() const;
    virtual bool isMet() const;
    virtual void addListener(std::shared_ptr<IRequirementListener> listener);
    virtual void removeListener(std::shared_ptr<IRequirementListener> listener);
    virtual MojErr toJson(MojObject& rep, unsigned flags) const;
    virtual pbnjson::JValue toJson() const;
    virtual void setSatisfied(bool satisfied);
    virtual void unsetSatisfied();

    // IRequirementListener
    virtual void onMetRequirement(std::string requirement);
    virtual void onUnmetRequirement(std::string requirement);
    virtual void onUpdateRequirement(std::string requirement);

private:
    std::shared_ptr<IRequirement> m_req;
    std::weak_ptr<IRequirementListener> m_listener;
    bool m_isUserDefined;
    bool m_isSatisfied;
};

#endif /* __PROXY_REQUIREMENT_H__ */
