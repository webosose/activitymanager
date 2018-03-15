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

#include <db/DB8CommandDB.h>
#include <db/DB8Manager.h>
#include <stdexcept>

#include "PersistTokenDB.h"
#include "conf/ActivityJson.h"
#include "util/Logging.h"

/*
 * palm://com.palm.db/put
 *
 * { "objects" :
 *    [{ "_kind" : "com.palm.activity:1", "_id" : "XXX", "_rev" : 123,
 *       "prop1" : VAL1, "prop2" : VAL2 },
 *     ... ]
 * }
 */
DB8StoreCommand::DB8StoreCommand(std::shared_ptr<Activity> activity,
                                             std::shared_ptr<ICompletion> completion)
    : DB8Command("luna://com.webos.service.db/put", activity, completion)
{
}

DB8StoreCommand::~DB8StoreCommand()
{
}

std::string DB8StoreCommand::getMethod() const
{
    return "Store";
}

void DB8StoreCommand::updateParams(MojObject& params)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [PersistCommand %s]: Updating parameters",
                 m_activity->getId(), getString().c_str());

    validate(false);

    MojErr errs = MojErrNone;
    MojErr err;
    MojObject rep;
    MojObject objectsArray;
    err = m_activity->toJson(rep, ACTIVITY_JSON_PERSIST | ACTIVITY_JSON_DETAIL);
    if (err) {
        throw std::runtime_error("Failed to convert Activity to JSON representation");
    }

    std::shared_ptr<PersistTokenDB> pt =
            std::dynamic_pointer_cast<PersistTokenDB, PersistToken>(m_activity->getPersistToken());
    if (pt->isValid()) {
        err = pt->toJson(rep);
        MojErrAccumulate(errs, err);
    }

    err = rep.putString(_T("_kind"), DB8Manager::kActivityKind);
    MojErrAccumulate(errs, err);

    err = objectsArray.push(rep);
    MojErrAccumulate(errs, err);

    err = params.put(_T("objects"), objectsArray);
    MojErrAccumulate(errs, err);

    if (errs) {
        LOG_AM_WARNING(MSGID_PERSIST_ATMPT_UNEXPECTD_EXCPTN, 3,
                       PMLOGKFV("Activity", "%llu", m_activity->getId()),
                       PMLOGKS("PersistCommand", getString().c_str()),
                       PMLOGKFV("err", "%d", errs),
                       "Failed to make DB update query");
    }
}

void DB8StoreCommand::persistResponse(MojServiceMessage *msg,
                                            const MojObject& response,
                                            MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [PersistCommand %s]: Processing response %s",
                 m_activity->getId(), getString().c_str(),
                 MojoObjectJson(response).c_str());

    try {
        validate(false);
    } catch (const std::exception& except) {
        LOG_AM_WARNING(MSGID_PERSIST_CMD_VALIDATE_EXCEPTION, 3,
                       PMLOGKFV("activity", "%llu", m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       PMLOGKS("exception",except.what()), "");
        complete(false);
        return;
    }

    if (err) {
        DB8Command::persistResponse(msg, response, err);
        return;
    }

    MojErr err2;
    MojString id;
    MojInt64 rev;

    bool found = false;
    MojObject resultArray;

    found = response.get(_T("results"), resultArray);
    if (!found) {
        LOG_AM_WARNING(MSGID_PERSIST_CMD_NO_RESULTS, 2,
                       PMLOGKFV("activity", "%llu", m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       "Results of MojoDB persist command not found in response");
        complete(false);
        return;
    }

    if (resultArray.arrayBegin() == resultArray.arrayEnd()) {
        LOG_AM_WARNING(MSGID_PERSIST_CMD_EMPTY_RESULTS, 0,
                       "MojoDB persist command returned empty result set");
        complete(false);
        return;
    }

    const MojObject& results = *(resultArray.arrayBegin());

    err2 = results.get(_T("id"), id, found);
    if (err2) {
        LOG_AM_WARNING(MSGID_PERSIST_CMD_GET_ID_ERR, 2,
                       PMLOGKFV("activity", "%llu", m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       "Error retreiving _id from MojoDB persist command response");
        complete(false);
        return;
    }

    if (!found) {
        LOG_AM_WARNING(MSGID_PERSIST_CMD_NO_ID, 2,
                       PMLOGKFV("activity", "%llu", m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       "_id not found in MojoDB persist command response");
        complete(false);
        return;
    }

    found = results.get(_T("rev"), rev);
    if (!found) {
        LOG_AM_ERROR(MSGID_PERSIST_CMD_RESP_REV_NOT_FOUND, 2,
                     PMLOGKFV("activity", "%llu", m_activity->getId()),
                     PMLOGKS("persist_command", getString().c_str()),
                     "_rev not found in MojoDB persist command response");
        complete(false);
        return;
    }

    std::shared_ptr<PersistTokenDB> pt =
            std::dynamic_pointer_cast<PersistTokenDB, PersistToken>(m_activity->getPersistToken());

    try {
        if (!pt->isValid()) {
            pt->set(id, rev);
        } else {
            pt->update(id, rev);
        }
    } catch (...) {
        LOG_AM_ERROR(MSGID_PERSIST_TOKEN_VAL_UPDATE_FAIL, 2,
                     PMLOGKFV("activity","%llu",m_activity->getId()),
                     PMLOGKS("persist_command",getString().c_str()),
                     "Failed to set or update value of persist token");
        complete(false);
        return;
    }

    DB8Command::persistResponse(msg, response, err);
}

/*
 * palm://com.palm.db/del
 *
 * { "ids" : [{ "XXX", "XXY", "XXZ" }] }
 */
DB8DeleteCommand::DB8DeleteCommand(std::shared_ptr<Activity> activity,
                                               std::shared_ptr<ICompletion> completion)
    : DB8Command("luna://com.webos.service.db/del", activity, completion)
{
}

DB8DeleteCommand::~DB8DeleteCommand()
{
}

std::string DB8DeleteCommand::getMethod() const
{
    return "Delete";
}

void DB8DeleteCommand::updateParams(MojObject& params)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [PersistCommand %s]: Updating parameters",
                 m_activity->getId(), getString().c_str());

    validate(true);

    std::shared_ptr<PersistTokenDB> pt =
            std::dynamic_pointer_cast<PersistTokenDB, PersistToken>(m_activity->getPersistToken());

    MojObject idsArray;
    MojErr errs = MojErrNone;
    MojErr err = idsArray.push(pt->getId());
    MojErrAccumulate(errs, err);

    err = params.put(_T("ids"), idsArray);
    MojErrAccumulate(errs, err);

    if (errs) {
        LOG_AM_WARNING(MSGID_PERSIST_ATMPT_UNEXPECTD_EXCPTN, 3,
                       PMLOGKFV("Activity", "%llu", m_activity->getId()),
                       PMLOGKS("PersistCommand", getString().c_str()),
                       PMLOGKFV("err", "%d", errs),
                       "Failed to make DB update query");
    }
}

void DB8DeleteCommand::persistResponse(MojServiceMessage *msg,
                                             const MojObject& response,
                                             MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [PersistCommand %s]: Processing response %s",
                 m_activity->getId(), getString().c_str(),
                 MojoObjectJson(response).c_str());

    if (!err) {
        std::shared_ptr<PersistTokenDB> pt =
                std::dynamic_pointer_cast<PersistTokenDB, PersistToken>(m_activity->getPersistToken());

        pt->clear();
    }

    DB8Command::persistResponse(msg, response, err);
}

