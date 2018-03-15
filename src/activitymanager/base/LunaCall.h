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

#ifndef __LUNA_CALL_H__
#define __LUNA_CALL_H__

#include <core/MojService.h>
#include <core/MojObject.h>
#include <core/MojServiceRequest.h>

#include "Main.h"
#include "activity/Activity.h"
#include "base/LunaURL.h"

class Activity;

class LunaCall: public std::enable_shared_from_this<LunaCall> {
public:
    LunaCall(bool isClient, const LunaURL& url, const MojObject& params, MojUInt32 replies = 1);
    virtual ~LunaCall();

    static const int kMaxRetry;
    static const MojUInt32 kUnlimited;
    static const std::string kServiceStatusReady;
    static const std::string kServiceStatusRunning;
    static const char* kBusErrorServiceBusy;

    MojErr call();
    MojErr call(std::shared_ptr<Activity> activity);
    MojErr call(std::string proxyRequester);

    MojErr retry();
    MojErr callStatus();

    void cancel();

    unsigned getSerial() const;

    static bool isPermanentFailure(MojServiceMessage *msg, const MojObject& response, MojErr err);
    static bool isPermanentBusFailure(MojServiceMessage *msg, const MojObject& response, MojErr err);
    static bool isBusError(MojServiceMessage *msg, const MojObject& response, std::string& error);

protected:
    static bool isProtocolError(MojServiceMessage *msg, const MojObject& response, MojErr err);

    void handleResponseWrapper(MojServiceMessage *msg, MojObject& response, MojErr err);
    void handleStatusResponseWrapper(MojServiceMessage *msg, MojObject& response, MojErr err);

    virtual void handleResponse(MojObject& response, MojErr err);
    virtual void handleResponse(MojServiceMessage *msg, MojObject& response, MojErr err);

    class LunaCallMessageHandler: public MojSignalHandler {
    public:
        typedef void (LunaCall::*Callback)(MojServiceMessage *msg, MojObject& response, MojErr err);

        LunaCallMessageHandler(std::shared_ptr<LunaCall> call, unsigned serial,
                               Callback callback = &LunaCall::handleResponseWrapper);
        ~LunaCallMessageHandler();

        MojServiceRequest::ExtendedReplySignal::Slot<LunaCallMessageHandler>& getResponseSlot();
        void cancel();

    protected:
        MojErr handleResponse(MojServiceMessage *msg, MojObject& response, MojErr err);

        std::weak_ptr<LunaCall> m_call;
        MojServiceRequest::ExtendedReplySignal::Slot<LunaCallMessageHandler> m_responseSlot;
        unsigned m_serial;
        Callback m_callback;
    };

    friend class LunaCallMessageHandler;

    bool m_isClient;
    LunaURL m_url;
    MojObject m_params;

    MojUInt32 m_replies;

    MojRefCountedPtr<LunaCall::LunaCallMessageHandler> m_handler;
    MojRefCountedPtr<LunaCall::LunaCallMessageHandler> m_statusHandler;

    std::string m_proxyRequester;

    unsigned m_serial;
    unsigned m_subSerial;

    int m_retryCount;

    static unsigned s_serial;
};

template<class T>
class LunaWeakPtrCall: public LunaCall {
public:
    typedef void (T::*CallbackType)(MojServiceMessage *msg, const MojObject& response, MojErr err);

    LunaWeakPtrCall(std::shared_ptr<T> ptr,
                    CallbackType callback,
                    bool isClient,
                    const LunaURL& url,
                    const MojObject& params,
                    MojUInt32 replies = 1)
        : LunaCall(isClient, url, params, replies), m_ptr(ptr), m_callback(callback)
    {
    }

    virtual ~LunaWeakPtrCall()
    {
    }

protected:
    virtual void handleResponse(MojServiceMessage *msg, MojObject& response, MojErr err)
    {
        if (!m_ptr.expired()) {
            ((*m_ptr.lock().get()).*m_callback)(msg, response, err);
        }
    }

    std::weak_ptr<T> m_ptr;
    CallbackType m_callback;
};

template<class T>
class LunaPtrCall: public LunaCall {
public:
    typedef void (T::*CallbackType)(MojServiceMessage *msg, const MojObject& response, MojErr err);

    LunaPtrCall(T* ptr,
                CallbackType callback,
                bool isClient,
                const LunaURL& url,
                const MojObject& params,
                MojUInt32 replies = 1)
        : LunaCall(isClient, url, params, replies), m_ptr(ptr), m_callback(callback)
    {
    }

    virtual ~LunaPtrCall()
    {
    }

protected:
    virtual void handleResponse(MojServiceMessage *msg, MojObject& response, MojErr err)
    {
        if (m_ptr) {
            (m_ptr->*m_callback)(msg, response, err);
        }
    }

    T* m_ptr;
    CallbackType m_callback;
};

#endif /* __LUNA_CALL_H__ */
