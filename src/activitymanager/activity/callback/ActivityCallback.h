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

#ifndef __ACTIVITY_CALLBACK_H__
#define __ACTIVITY_CALLBACK_H__

#include <core/MojService.h>

#include "base/AbstractCallback.h"
#include "base/LunaCall.h"

class ActivityCallback : public AbstractCallback
{
public:
    ActivityCallback(std::shared_ptr<Activity> activity,
                     const LunaURL& url,
                     const MojObject& params,
                     bool ignoreReturnValue);
    virtual ~ActivityCallback();

    virtual MojErr call(MojObject activityInfo);
    virtual void cancel();

    virtual void failed(FailureType failure);
    virtual void succeeded();

    const LunaURL& getMethod() const { return m_url; }
    const MojObject& getParams() const { return m_params; }

    virtual MojErr toJson(MojObject& rep, unsigned flags) const;

protected:
    virtual void handleResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);

    LunaURL m_url;
    MojObject m_params;
    bool m_ignoreReturnValue;

    std::shared_ptr<LunaWeakPtrCall<ActivityCallback>> m_call;
};

#endif /* __ACTIVITY_CALLBACK_H__ */
