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

#ifndef __TIMEOUT_H__
#define __TIMEOUT_H__

#include <glib.h>

#include "Main.h"

class TimeoutBase {
public:
    TimeoutBase(unsigned seconds);
    virtual ~TimeoutBase();

    void arm();
    void cancel();

protected:
    virtual void wakeupTimeout() = 0;
    static gboolean staticWakeupTimeout(gpointer data);

    unsigned m_seconds;
    GSource *m_timeout;

    static MojLogger s_log;
};

template <class T>
class Timeout : public TimeoutBase {
public:
    typedef void (T::* CallbackType)();

    Timeout(std::shared_ptr<T> target, unsigned seconds, CallbackType callback)
        : TimeoutBase(seconds)
        , m_callback(callback)
        , m_target(target)
    {
    }

    virtual ~Timeout() {}

    virtual void wakeupTimeout()
    {
        if (!m_target.expired()) {
            ((*m_target.lock().get()).*m_callback)();
        }
    }

protected:
    CallbackType m_callback;
    std::weak_ptr<T> m_target;
};

template <class T>
class TimeoutPtr : public TimeoutBase {
public:
    typedef void (T::* CallbackType)();

    TimeoutPtr(T* target, unsigned seconds, CallbackType callback)
        : TimeoutBase(seconds)
        , m_callback(callback)
        , m_target(target)
    {
    }

    virtual ~TimeoutPtr() {}

    virtual void wakeupTimeout()
    {
        if (m_target) {
            (*m_target.*m_callback)();
        }
    }

protected:
    CallbackType m_callback;
    T* m_target;
};

#endif /* __TIMEOUT_H__ */
