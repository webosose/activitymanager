// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#ifndef __PERMISSION_MANAGER_H__
#define __PERMISSION_MANAGER_H__

#include <deque>

#include <core/MojService.h>
#include <core/MojServiceRequest.h>

#include "activity/Activity.h"
#include "activity/callback/ActivityCallback.h"
#include "base/AbstractCallback.h"
#include "base/LunaURL.h"

class PermissionManager: public std::enable_shared_from_this<PermissionManager> {
public:
    typedef std::function<MojErr (MojErr, std::string)> PermissionCallback;

    PermissionManager();
    virtual ~PermissionManager();

    MojErr syncCheck(const std::string& requester, const std::string& url);

    void checkAccessRight(std::string requester, std::vector<std::string> urls,
                          PermissionCallback callback);
    static std::string getRequester(MojRefCountedPtr<MojServiceMessage> msg);


private:
    struct SyncChecker: public MojSignalHandler {
        typedef MojServiceRequest::ExtendedReplySignal::Slot<SyncChecker> HandlerType;

        SyncChecker();
        virtual ~SyncChecker();

        MojErr check(const std::string& requester, const std::string& url);
        MojErr receiveResponse(MojServiceMessage *, MojObject& response, MojErr);
        static int timeout(void* ctx);

        HandlerType m_slot;
        volatile bool m_received;
        MojObject m_response;
        unsigned m_timerId;
    };

    static int processQueue(void* ctx);

    std::deque<std::tuple<std::string, std::vector<std::string>,
            PermissionManager::PermissionCallback>> m_queue;
};

#endif //__PERMISSION_MANAGER_H__
