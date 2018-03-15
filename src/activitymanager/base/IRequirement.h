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

#ifndef __IREQUIREMENT_H__
#define __IREQUIREMENT_H__

#include <memory>

#include <core/MojErr.h>
#include <core/MojObject.h>

class IRequirementListener {
public:
    IRequirementListener() {};
    virtual ~IRequirementListener() {};

    virtual void onMetRequirement(std::string requirement) = 0;
    virtual void onUnmetRequirement(std::string requirement) = 0;
    virtual void onUpdateRequirement(std::string requirement) = 0;
};

class IRequirement {
public:
    IRequirement() {};
    virtual ~IRequirement() {};

    virtual const std::string& getName() const = 0;

    virtual bool isMet() const = 0;

    virtual void addListener(std::shared_ptr<IRequirementListener> listener) = 0;
    virtual void removeListener(std::shared_ptr<IRequirementListener> listener) = 0;

    virtual MojErr toJson(MojObject& rep, unsigned flags) const = 0;
    virtual pbnjson::JValue toJson() const = 0;

    virtual void setSatisfied(bool satisfied) = 0;
    virtual void unsetSatisfied() = 0;
};

#endif /* __IREQUIREMENT_H__ */
