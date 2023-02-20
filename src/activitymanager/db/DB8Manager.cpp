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

#include <stdexcept>
#include <core/MojObject.h>
#include <db/DB8CommandDB.h>
#include <db/DB8Manager.h>
#include <db/MojDbQuery.h>

#include "conf/ActivityJson.h"
#include "service/BusConnection.h"
#include "util/Logging.h"

const char *DB8Manager::kActivityKind = _T("com.webos.service.activity:1");

const int DB8Manager::kPurgeBatchSize = MojDbQuery::MaxQueryLimit;

DB8Manager::DB8Manager()
    : m_listener(NULL)
{
}

DB8Manager::~DB8Manager()
{
}

std::shared_ptr<AbstractPersistCommand> DB8Manager::prepareStoreCommand(std::shared_ptr<Activity> activity,
                                                                     std::shared_ptr<ICompletion> completion)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Preparing put command for [Activity %llu]", activity->getId());

    return std::make_shared<DB8StoreCommand>(activity, completion);
}

std::shared_ptr<AbstractPersistCommand> DB8Manager::prepareDeleteCommand(std::shared_ptr<Activity> activity,
                                                                      std::shared_ptr<ICompletion> completion)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Preparing delete command for [Activity %llu]", activity->getId());

    return std::make_shared<DB8DeleteCommand>(activity, completion);
}

std::shared_ptr<PersistToken> DB8Manager::createToken()
{
    return std::make_shared<PersistTokenDB>();
}

void DB8Manager::loadActivities()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Loading persisted Activities from MojoDB");

    MojObject query;
    MojObject params;
    MojErr errs = MojErrNone;
    MojErr err = query.putString(_T("from"), kActivityKind);
    MojErrAccumulate(errs, err);

    err = params.put(_T("query"), query);
    MojErrAccumulate(errs, err);

    if (errs) {
        LOG_AM_WARNING(MSGID_ACTIVITIES_LOAD_ERR, 1, PMLOGKFV("err", "%d", errs),
                       "Failed to make DB load query");
    }

    m_call = std::make_shared<LunaPtrCall<DB8Manager>>(
            this,
            &DB8Manager::activityLoadResults,
            true,
            "luna://com.webos.service.db/find",
            params);

    m_call->call();
}

void DB8Manager::addListener(DB8ManagerListener* listener)
{
    m_listener = listener;
}

void DB8Manager::activityLoadResults(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Processing Activities loaded from MojoDB");

    /* Don't allow the Activity Manager to start up if the MojoDB load
     * fails ... */
    if (err != MojErrNone) {
        if (LunaCall::isPermanentFailure(msg, response, err)) {
            LOG_AM_ERROR(MSGID_LOAD_ACTIVITIES_FROM_DB_FAIL, 0,
                         "Uncorrectable error loading Activities from MojoDB: %s",
                         MojoObjectJson(response).c_str());
            if (m_listener) m_listener->onFail();
        } else {
            LOG_AM_WARNING(MSGID_ACTIVITIES_LOAD_ERR, 0,
                           "Error loading Activities from MojoDB, retrying: %s",
                           MojoObjectJson(response).c_str());
            m_call->call();
        }
        return;
    }

    /* Clear current call */
    m_call.reset();

    bool found;
    MojErr err2;
    MojObject results;
    found = response.get(_T("results"), results);

    if (found) {
        for (MojObject::ConstArrayIterator iter = results.arrayBegin() ;
                iter != results.arrayEnd() ; ++iter) {
            const MojObject& rep = *iter;
            MojInt64 activityId;

            bool found;
            found = rep.get(_T("activityId"), activityId);
            if (!found) {
                LOG_AM_WARNING(MSGID_ACTIVITYID_NOT_FOUND, 0,
                               "activityId not found loading Activities");
                continue;
            }

            MojString id;
            if (rep.get(_T("_id"), id, found)) {
                LOG_AM_WARNING(MSGID_RETRIEVE_ID_FAIL, 0,
                               "Error retrieving _id from results returned from MojoDB");
                continue;
            }

            if (!found) {
                LOG_AM_WARNING(MSGID_ID_NOT_FOUND, 0,
                               "_id not found loading Activities from MojoDB");
                continue;
            }

            MojInt64 rev;
            found = rep.get(_T("_rev"), rev);
            if (!found) {
                LOG_AM_WARNING(MSGID_REV_NOT_FOUND, 0,
                               "_rev not found loading Activities from MojoDB");
                continue;
            }

            std::shared_ptr<PersistTokenDB> pt = std::make_shared<PersistTokenDB>(id, rev);

            std::shared_ptr<Activity> act;

            try {
                act = ActivityExtractor::createActivity(rep, true);
            } catch (const std::exception& except) {
                LOG_AM_WARNING(MSGID_CREATE_ACTIVITY_EXCEPTION, 1,
                               PMLOGKS("Exception",except.what()), "Activity: %s",
                               MojoObjectJson(rep).c_str());
                m_oldTokens.push_back(pt);
                continue;
            } catch (...) {
                LOG_AM_WARNING(MSGID_UNKNOWN_EXCEPTION, 0,
                               "Activity : %s. Unknown exception decoding encoded",
                               MojoObjectJson(rep).c_str());
                m_oldTokens.push_back(pt);
                continue;
            }

            act->setPersistToken(pt);

            /* Attempt to register this Activity's Id and Name, in order. */

            try {
                ActivityManager::getInstance().registerActivityId(act);
            } catch (...) {
                LOG_AM_ERROR(MSGID_ACTIVITY_ID_REG_FAIL, 1,
                             PMLOGKFV("Activity", "%llu", act->getId()), "");

                /* Another Activity is already registered.  Determine which
                 * is newer, and kill the older one. */

                std::shared_ptr<Activity> old = ActivityManager::getInstance().getActivity(act->getId());
                std::shared_ptr<PersistTokenDB> oldPt =
                        std::dynamic_pointer_cast<PersistTokenDB, PersistToken>(old->getPersistToken());
                if (oldPt && pt && act) {
                    if (pt->getRev() > oldPt->getRev()) {
                        LOG_AM_WARNING(
                                MSGID_ACTIVITY_REPLACED,
                                4,
                                PMLOGKFV("Activity", "%llu", act->getId()),
                                PMLOGKFV("revision", "%llu", (unsigned long long)pt->getRev()),
                                PMLOGKFV("old_Activity", "%llu", old->getId()),
                                PMLOGKFV("old_revision", "%llu", (unsigned long long)oldPt->getRev()),
                                "");

                        m_oldTokens.push_back(oldPt);
                        ActivityManager::getInstance().unregisterActivityName(old);
                        ActivityManager::getInstance().releaseActivity(old);

                        ActivityManager::getInstance().registerActivityId(act);
                    } else {
                        LOG_AM_WARNING(
                                MSGID_ACTIVITY_NOT_REPLACED,
                                4,
                                PMLOGKFV("Activity", "%llu", act->getId()),
                                PMLOGKFV("revision", "%llu", (unsigned long long)pt->getRev()),
                                PMLOGKFV("old_Activity", "%llu", old->getId()),
                                PMLOGKFV("old_revision", "%llu", (unsigned long long)oldPt->getRev()),
                                "");

                        m_oldTokens.push_back(pt);
                        ActivityManager::getInstance().releaseActivity(act);
                        continue;
                    }
                }
            }

            try {
                ActivityManager::getInstance().registerActivityName(act);
            } catch (...) {
                LOG_AM_ERROR(
                        MSGID_ACTIVITY_NAME_REG_FAIL, 3,
                        PMLOGKFV("Activity", "%llu", act->getId()),
                        PMLOGKS("Creator_name", act->getCreator().getString().c_str()),
                        PMLOGKS("Register_name", act->getName().c_str()), "");

                /* Another Activity is already registered.  Determine which
                 * is newer, and kill the older one. */

                std::shared_ptr<Activity> old =
                        ActivityManager::getInstance().getActivity(act->getName(), act->getCreator());

                std::shared_ptr<PersistTokenDB> oldPt =
                        std::dynamic_pointer_cast<PersistTokenDB, PersistToken>(old->getPersistToken());
                if (oldPt && pt && act) {
                    if (pt->getRev() > oldPt->getRev()) {
                        LOG_AM_WARNING(
                                MSGID_ACTIVITY_REPLACED,
                                4,
                                PMLOGKFV("Activity","%llu",act->getId()),
                                PMLOGKFV("revision","%llu", (unsigned long long)pt->getRev()),
                                PMLOGKFV("old_Activity","%llu", old->getId()),
                                PMLOGKFV("old_revision","%llu", (unsigned long long)oldPt->getRev()),
                                "");

                        m_oldTokens.push_back(oldPt);
                        ActivityManager::getInstance().unregisterActivityName(old);
                        ActivityManager::getInstance().releaseActivity(old);

                        ActivityManager::getInstance().registerActivityName(act);
                    } else {
                        LOG_AM_WARNING(
                                MSGID_ACTIVITY_NOT_REPLACED,
                                4,
                                PMLOGKFV("Activity","%llu", act->getId()),
                                PMLOGKFV("revision","%llu", (unsigned long long)pt->getRev()),
                                PMLOGKFV("old_Activity","%llu", old->getId()),
                                PMLOGKFV("old_revision","%llu", (unsigned long long)oldPt->getRev()),
                                "");

                        m_oldTokens.push_back(pt);
                        ActivityManager::getInstance().releaseActivity(act);
                        continue;
                    }
                }
            }

            LOG_AM_DEBUG("[Activity %llu] (\"%s\"): _id %s, rev %llu loaded",
                         act->getId(), act->getName().c_str(), id.data(),
                         (unsigned long long )rev);

            /* Request Activity be scheduled.  It won't transition to running
             * until after the MojoDB load finishes (and the Activity Manager
             * moves to the ready() and start()ed states). */
            ActivityManager::getInstance().startActivity(act);
        }
    }

    MojString page;
    MojErr errs = MojErrNone;
    err2 = response.get(_T("page"), page, found);
    if (err2) {
        LOG_AM_ERROR(MSGID_GET_PAGE_FAIL, 0,
                     "Error getting page parameter in MojoDB query response");
        return;
    }

    if (found) {
        LOG_AM_DEBUG("Preparing to request next page (\"%s\") of Activities",
                     page.data());

        MojObject query;
        err2 = query.putString(_T("from"), kActivityKind);
        MojErrAccumulate(errs, err2);

        err2 = query.putString(_T("page"), page);
        MojErrAccumulate(errs, err2);

        MojObject params;
        err2 = params.put(_T("query"), query);
        MojErrAccumulate(errs, err2);

        if (errs) {
            LOG_AM_WARNING(MSGID_ACTIVITIES_LOAD_ERR, 1, PMLOGKFV("err", "%d", errs),
                           "Failed to make DB paged load query");
        }

        m_call = std::make_shared<LunaPtrCall<DB8Manager>>(
                this,
                &DB8Manager::activityLoadResults,
                true,
                "luna://com.webos.service.db/find",
                params);

        m_call->call();
    } else {
        LOG_AM_DEBUG("All Activities successfully loaded from MojoDB");

        if (!m_oldTokens.empty()) {
            LOG_AM_DEBUG("Beginning purge of old Activities from database");
            preparePurgeCall();
            m_call->call();
        } else {
            m_call.reset();
            ActivityManager::getInstance().enable(ActivityManager::kConfigurationLoaded);
        }

        if (m_listener) m_listener->onFinish();
    }
}

void DB8Manager::activityPurgeComplete(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Purge of batch of old Activities complete");

    /* If there was a transient error, re-call */
    if (err != MojErrNone) {
        if (LunaCall::isPermanentFailure(msg, response, err)) {
            LOG_AM_ERROR(MSGID_ACTIVITIES_PURGE_FAILED, 0,
                         "Purge of batch of old Activities failed: %s",
                         MojoObjectJson(response).c_str());
            m_call.reset();
        } else {
            LOG_AM_WARNING(MSGID_RETRY_ACTIVITY_PURGE, 0,
                           "Purge of batch of old Activities failed, retrying: %s",
                           MojoObjectJson(response).c_str());
            m_call->call();
        }
        return;
    }

    if (!m_oldTokens.empty()) {
        preparePurgeCall();
        m_call->call();
    } else {
        LOG_AM_DEBUG("Done purging old Activities");
        m_call.reset();
        ActivityManager::getInstance().enable(ActivityManager::kConfigurationLoaded);
    }
}

void DB8Manager::activityConfiguratorComplete(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Load of static Activity configuration complete");

    if (err != MojErrNone) {
        LOG_AM_WARNING(MSGID_ACTIVITY_CONFIG_LOAD_FAIL, 0,
                       "Failed to load static Activity configuration: %s",
                       MojoObjectJson(response).c_str());
    }

    m_call.reset();
    ActivityManager::getInstance().enable(ActivityManager::kConfigurationLoaded);
}

void DB8Manager::preparePurgeCall()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Preparing to purge batch of old Activities");

    MojObject ids(MojObject::TypeArray);
    populatePurgeIds(ids);

    MojObject params;
    MojErr err = params.put(_T("ids"), ids);

    if (err) {
        LOG_AM_WARNING(MSGID_ACTIVITIES_PURGE_FAILED, 1, PMLOGKFV("err", "%d", err),
                       "Failed to make DB purge query");
    }

    m_call = std::make_shared<LunaPtrCall<DB8Manager>>(
            this,
            &DB8Manager::activityPurgeComplete,
            true,
            "luna://com.webos.service.db/del",
            params);
}

void DB8Manager::populatePurgeIds(MojObject& ids)
{
    int count = kPurgeBatchSize;

    while (!m_oldTokens.empty()) {
        std::shared_ptr<PersistTokenDB> pt = m_oldTokens.front();
        m_oldTokens.pop_front();

        if (ids.push(pt->getId()) != MojErrNone)
            LOG_AM_DEBUG("Failed to push id into MojObject");
        if (--count == 0)
            return;
    }
}
