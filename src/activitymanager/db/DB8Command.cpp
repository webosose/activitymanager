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

#include <db/DB8Command.h>
#include <stdexcept>

#include "util/Logging.h"
#include "util/MojoObjectString.h"
#include "service/BusConnection.h"

DB8Command::DB8Command(
        const LunaURL& method, std::shared_ptr<Activity> activity,
        std::shared_ptr<ICompletion> completion)
    : AbstractPersistCommand(activity, completion)
    , m_method(method)
{
}

DB8Command::~DB8Command()
{
}

void DB8Command::persist()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [PersistCommand %s]: Issuing",
                 m_activity->getId(), getString().c_str());

    /* Perform update of Parameters, if desired - modify copy, not original,
     * to ensure old data isn't retained between calls */
    try {
        MojObject params = m_params;
        updateParams(params);

        m_call = std::make_shared<LunaWeakPtrCall<DB8Command>>(
                std::dynamic_pointer_cast<DB8Command, AbstractPersistCommand>(shared_from_this()),
                &DB8Command::persistResponse,
                true, m_method, params);
        m_call->call();
    } catch (const std::exception& except) {
        LOG_AM_ERROR(MSGID_PERSIST_ATMPT_UNEXPECTD_EXCPTN, 3,
                     PMLOGKFV("activity", "%llu", m_activity->getId()),
                     PMLOGKS("Persist_command", getString().c_str()),
                     PMLOGKS("Exception", except.what()),
                     "Unexpected exception while attempting to persist");
        complete(false);
    } catch (...) {
        LOG_AM_ERROR(MSGID_PERSIST_ATMPT_UNKNWN_EXCPTN, 2,
                     PMLOGKFV("activity", "%llu", m_activity->getId()),
                     PMLOGKS("Persist_command", getString().c_str()),
                     "Unknown exception while attempting to persist");
        complete(false);
    }
}

void DB8Command::persistResponse(
        MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (err == MojErrNone) {
        LOG_AM_DEBUG("[Activity %llu] [PersistCommand %s]: Succeeded",
                        m_activity->getId(), getString().c_str());
        complete(true);
        return;
    }

    if (LunaCall::isPermanentFailure(msg, response, err)) {
        LOG_AM_WARNING(MSGID_PERSIST_CMD_RESP_FAIL, 4,
                PMLOGKFV("activity", "%llu", m_activity->getId()),
                PMLOGKS("persist_command", getString().c_str()),
                PMLOGKS("Errtext", MojoObjectString(response, _T("errorText")).c_str()),
                PMLOGKFV("Errcode", "%d", (int)err), "");
        complete(false);
    } else {
        LOG_AM_WARNING(MSGID_PERSIST_CMD_TRANSIENT_ERR, 2,
                PMLOGKFV("activity", "%llu", m_activity->getId()),
                PMLOGKS("persist_command", getString().c_str()),
                "Failed with transient error, retrying: %s", MojoObjectJson(response).c_str());
        m_call->call();
    }
}

