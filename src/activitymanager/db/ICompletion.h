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

#ifndef __COMPLETION_H__
#define __COMPLETION_H__

#include <core/MojServiceMessage.h>

#include "Main.h"

class Activity;

class ICompletion {
public:
    ICompletion() {};
    virtual ~ICompletion() {};

    virtual void complete(bool succeeded) = 0;
};

template <class T>
class MojoMsgCompletion : public ICompletion {
public:
    typedef void (T::* CallbackType)(MojRefCountedPtr<MojServiceMessage> msg,
            const MojObject& payload, std::shared_ptr<Activity> activity, bool succeeded);

    MojoMsgCompletion(T *category,
                      CallbackType callback,
                      MojServiceMessage *msg,
                      const MojObject& payload,
                      std::shared_ptr<Activity> activity)
        : m_payload(payload)
        , m_callback(callback)
        , m_category(category)
        , m_msg(msg)
        , m_activity(activity)
    {
    }

    virtual ~MojoMsgCompletion() {}

    virtual void complete(bool succeeded)
    {
        ((*m_category.get()).*m_callback)(m_msg, m_payload, m_activity, succeeded);
    }

protected:
    MojObject m_payload;
    CallbackType m_callback;

    MojRefCountedPtr<T> m_category;
    MojRefCountedPtr<MojServiceMessage> m_msg;

    std::shared_ptr<Activity> m_activity;
};

template<class T>
class MojoRefCompletion : public ICompletion {
public:
    typedef void (T::* CallbackType)(std::shared_ptr<Activity> activity, bool succeeded);

    MojoRefCompletion(T *target,
                      CallbackType callback,
                      std::shared_ptr<Activity> activity)
        : m_callback(callback)
        , m_target(target)
        , m_activity(activity)
    {
    }

    virtual ~MojoRefCompletion() {}

    virtual void complete(bool succeeded)
    {
        ((*m_target.get()).*m_callback)(m_activity, succeeded);
    }

protected:
    CallbackType m_callback;
    MojRefCountedPtr<T> m_target;
    std::shared_ptr<Activity> m_activity;
};

#endif /* __COMPLETION_H__ */
