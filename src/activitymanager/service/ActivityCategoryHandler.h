// Copyright (c) 2009-2023 LG Electronics, Inc.
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

#ifndef __ACTIVITY_CATEGORY_HANDLER_H__
#define __ACTIVITY_CATEGORY_HANDLER_H__

#include <activity/trigger/TriggerFactory.h>
#include <boost/function.hpp>
#include <core/MojService.h>
#include <core/MojServiceMessage.h>
#include <db/AbstractPersistManager.h>
#include "Main.h"

#include "activity/Activity.h"
#include "activity/ActivityExtractor.h"
#include "activity/ActivityManager.h"
#include "base/AbstractCallback.h"
#include "base/ITrigger.h"
#include "service/PermissionManager.h"
#include "service/Subscription.h"

class ActivityCategoryHandler : public MojService::CategoryHandler {
private:
    static const MojChar* const CreateSchema;
    static const MojChar* const ReleaseSchema;
    static const MojChar* const PauseSchema;
    static const MojChar* const AdoptSchema;
    static const MojChar* const CompleteSchema;
    static const MojChar* const StartSchema;
    static const MojChar* const StopSchema;
    static const MojChar* const CancelSchema;
    static const MojChar* const GetActivityInfoSchema;

public:
    ActivityCategoryHandler(std::shared_ptr<PermissionManager> pm);

    virtual ~ActivityCategoryHandler();

    MojErr init();

    MojErr replyDeprecatedMethod(MojServiceMessage *msg, MojObject& payload);

protected:
    /* Lifecycle Methods */
    MojErr createActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr releaseActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr adoptActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr completeActivity(MojServiceMessage *msg, MojObject& payload);

    MojErr finishCreateActivityPermissionCheck(
            MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload,
            std::shared_ptr<Activity> act, MojErr errorCode, std::string errorText);
    MojErr finishCompleteActivityPermissionCheck(
            MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload,
            std::shared_ptr<Activity> act, bool restart, MojErr errorCode, std::string errorText);

    /* Control Methods */
    MojErr scheduleActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr startActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr stopActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr cancelActivity(MojServiceMessage *msg, MojObject& payload);
    MojErr pauseActivity(MojServiceMessage *msg, MojObject& payload);

    /* Completion methods */
    void finishCreateActivity(MojRefCountedPtr<MojServiceMessage> msg,
                              const MojObject& payload,
                              std::shared_ptr<Activity> act,
                              bool succeeded);
    void finishReplaceActivity(std::shared_ptr<Activity> act, bool succeeded);
    void finishCompleteActivity(MojRefCountedPtr<MojServiceMessage> msg,
                                const MojObject& payload,
                                std::shared_ptr<Activity> act,
                                bool succeeded);
    void finishStopActivity(MojRefCountedPtr<MojServiceMessage> msg,
                            const MojObject& payload,
                            std::shared_ptr<Activity> act,
                            bool succeeded);
    void finishCancelActivity(MojRefCountedPtr<MojServiceMessage> msg,
                              const MojObject& payload,
                              std::shared_ptr<Activity> act,
                              bool succeeded);

    /* Introspection */
    MojErr getActivityInfo(MojServiceMessage *msg, MojObject& payload);

    /* Debugging/Information Methods */
    MojErr getManagerInfo(MojServiceMessage *msg, MojObject& payload);

    /* External Enable/Disable of for scheduling new Activities */
    MojErr enable(MojServiceMessage *msg, MojObject& payload);
    MojErr disable(MojServiceMessage *msg, MojObject& payload);

    /* Service Methods */
    MojErr lookupActivity(MojServiceMessage *msg,
                          MojObject& payload,
                          std::shared_ptr<Activity>& act);

    bool validateCaller(BusId caller, std::shared_ptr<Activity>& act);

    bool subscribeActivity(MojServiceMessage *msg,
                           const MojObject& payload,
                           const std::shared_ptr<Activity>& act,
                           std::shared_ptr<Subscription>& sub);

    MojErr checkSerial(MojServiceMessage *msg,
                       const MojObject& payload,
                       const std::shared_ptr<Activity>& act);

    typedef void (ActivityCategoryHandler::*CompletionFunction)(MojRefCountedPtr<MojServiceMessage> msg,
                                                                const MojObject& payload,
                                                                std::shared_ptr<Activity> act,
                                                                bool succeeded);

    void ensureCompletion(MojServiceMessage *msg,
                          MojObject& payload,
                          AbstractPersistManager::CommandType type,
                          CompletionFunction func,
                          std::shared_ptr<Activity> act);
    void ensureReplaceCompletion(MojServiceMessage *msg,
                                 MojObject& payload,
                                 std::shared_ptr<Activity> oldActivity,
                                 std::shared_ptr<Activity> newActivity);

    void checkAccessRight(const std::string& requester,
                          std::shared_ptr<AbstractCallback> c,
                          std::vector<std::shared_ptr<ITrigger>> vec,
                          PermissionManager::PermissionCallback permissionCB);

    static const SchemaMethod s_methods[];

    std::shared_ptr<PermissionManager> m_pm;
};

#endif /* __ACTIVITY_CATEGORY_HANDLER_H__ */
