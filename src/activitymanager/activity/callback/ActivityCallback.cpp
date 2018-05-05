// Copyright (c) 2009-2018 LG Electronics, Inc.
//
//      Copyright (c) 2009-2018 LG Electronics, Inc.
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

#include "ActivityCallback.h"
#include "activity/Activity.h"
#include "conf/ActivityJson.h"
#include "service/BusConnection.h"
#include "util/Logging.h"

ActivityCallback::ActivityCallback(std::shared_ptr<Activity> activity,
                                   const LunaURL& url,
                                   const MojObject& params,
                                   bool ignoreReturnValue)
    : AbstractCallback(activity)
    , m_url(url)
    , m_params(params)
    , m_ignoreReturnValue(ignoreReturnValue)
{
}

ActivityCallback::~ActivityCallback()
{
}

MojErr ActivityCallback::call(MojObject activityInfo)
{
    std::shared_ptr<Activity> activity;
    MojObject params = m_params;
    MojErr err;

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    activity = m_activity.lock();

    /* Update the command sequence Serial number */
    setSerial((unsigned) ::random() % UINT_MAX);

    LOG_AM_DEBUG("[Activity %llu] Callback %s: Calling [Serial %u]",
                 activity->getId(), m_url.getString().c_str(), getSerial());

    if (activityInfo == MojObject::Undefined) {
        err = activity->activityInfoToJson(activityInfo);
        MojErrCheck(err);
    }

    err = params.put(_T("$activity"), activityInfo);
    MojErrCheck(err);

    /* If an old MojoCall existed, it will be cancelled when its ref count
     * drops... */
    m_call = std::make_shared<LunaWeakPtrCall<ActivityCallback>>(
            std::dynamic_pointer_cast<ActivityCallback, AbstractCallback>(shared_from_this()),
            &ActivityCallback::handleResponse, false, m_url, params);
    m_call->call(activity);

    return MojErrNone;
}

void ActivityCallback::cancel()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Callback %s: Cancelling",
                 m_activity.lock()->getId(), m_url.getString().c_str());

    if (m_call) {
        m_call.reset();
    }
}

void ActivityCallback::failed(FailureType failure)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Callback %s: Failed%s",
                 m_activity.lock()->getId(), m_url.getString().c_str(),
                 (failure == TransientFailure) ? " (transient)" : "");

    m_call.reset();
    AbstractCallback::failed(failure);
}

void ActivityCallback::succeeded()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Callback %s: Succeeded",
                 m_activity.lock()->getId(), m_url.getString().c_str());

    m_call.reset();
    AbstractCallback::succeeded();
}

void ActivityCallback::handleResponse(MojServiceMessage *msg, const MojObject& rep, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Callback %s: Response %s",
                 m_activity.lock()->getId(), m_url.getString().c_str(),
                 MojoObjectJson(rep).c_str());

    if (MojErrNone == err) {
        succeeded();
        return;
    }

    std::string error;
    if (LunaCall::isBusError(msg, rep, error)) {
        if (LunaCall::isPermanentBusFailure(msg, rep, err)) {
            failed(PermanentFailure);
        } else {
            failed(TransientFailure);
        }
    } else if (m_ignoreReturnValue) {
        succeeded();
    } else {
        failed(PermanentFailure);
    }
}

MojErr ActivityCallback::toJson(MojObject& rep, unsigned flags) const
{
    MojErr err;

    if (flags & ACTIVITY_JSON_CURRENT) {
        unsigned serial = getSerial();
        if (serial != AbstractCallback::kUnassignedSerial) {
            err = rep.putInt(_T("serial"), (MojInt64) serial);
            MojErrCheck(err);
        }
    } else {
        err = rep.putString(_T("method"), m_url.getString().c_str());
        MojErrCheck(err);

        if (m_params.type() != MojObject::TypeUndefined) {
            err = rep.put(_T("params"), m_params);
            MojErrCheck(err);
        }
    }

    return MojErrNone;
}

