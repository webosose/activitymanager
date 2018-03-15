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

#ifndef __ITRIGGER_H__
#define __ITRIGGER_H__

#include <pbnjson.hpp>
#include <memory>

#include "Main.h"

class TriggerSubscription;

class ITrigger {
public:
    ITrigger() {}
    virtual ~ITrigger() {}

    virtual void init() = 0;
    virtual void clear() = 0;
    virtual bool isInit() const = 0;

    virtual void setName(const std::string& name) = 0;
    virtual const std::string& getName() const = 0;

    virtual std::shared_ptr<TriggerSubscription> getSubscription() const = 0;
    virtual int getSubscriptionCount() = 0;

    virtual bool isSatisfied() const = 0;

    virtual MojErr toJson(MojObject& rep, unsigned flags) const = 0;
    virtual pbnjson::JValue toJson() const = 0;

    virtual void setSatisfied(bool satisfied) = 0;
    virtual void unsetSatisfied() = 0;
};

#endif /* __ITRIGGER_H__ */
