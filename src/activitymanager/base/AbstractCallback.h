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

#ifndef __ABSTRACT_CALLBACK_H__
#define __ABSTRACT_CALLBACK_H__

#include <climits>

#include "Main.h"

class Activity;

class AbstractCallback : public std::enable_shared_from_this<AbstractCallback> {
public:
    AbstractCallback(std::shared_ptr<Activity> activity);
    virtual ~AbstractCallback();

    virtual MojErr call(MojObject activityInfo = MojObject::Undefined) = 0;
    virtual void cancel() = 0;

    enum FailureType {
        PermanentFailure,
        TransientFailure
    };

    virtual void failed(FailureType failure);
    virtual void succeeded();

    unsigned getSerial() const;
    void setSerial(unsigned serial);

    virtual MojErr toJson(MojObject& rep, unsigned flags) const = 0;

    static const unsigned kUnassignedSerial = UINT_MAX;

protected:
    unsigned m_serial;

    std::weak_ptr<Activity> m_activity;
};

#endif /* __ABSTRACT_CALLBACK_H__ */
