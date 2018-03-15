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

#ifndef _DB_MANAGER_H_
#define _DB_MANAGER_H_

#include <db/AbstractPersistManager.h>
#include <db/ICompletion.h>
#include <list>

#include "activity/Activity.h"
#include "activity/ActivityExtractor.h"
#include "activity/ActivityManager.h"
#include "base/LunaCall.h"
#include "db/PersistTokenDB.h"

class DB8ManagerListener {
public:
    DB8ManagerListener() {};
    virtual ~DB8ManagerListener() {};

    virtual void onFinish() = 0;
    virtual void onFail() = 0;
};

class DB8Manager: public AbstractPersistManager {
public:
    static DB8Manager& getInstance()
    {
        static DB8Manager _instance;
        return _instance;
    }

    virtual ~DB8Manager();

    virtual std::shared_ptr<AbstractPersistCommand> prepareStoreCommand(
            std::shared_ptr<Activity> activity, std::shared_ptr<ICompletion> completion);
    virtual std::shared_ptr<AbstractPersistCommand> prepareDeleteCommand(
            std::shared_ptr<Activity> activity, std::shared_ptr<ICompletion> completion);

    virtual std::shared_ptr<PersistToken> createToken();

    virtual void loadActivities();
    virtual void addListener(DB8ManagerListener* listener);

    static const char *kActivityKind;

protected:
    DB8Manager();
    void activityLoadResults(MojServiceMessage *msg, const MojObject& response, MojErr err);
    void activityPurgeComplete(MojServiceMessage *msg, const MojObject& response, MojErr err);
    void activityConfiguratorComplete(MojServiceMessage *msg, const MojObject& response, MojErr err);

    static const int kPurgeBatchSize;

    void preparePurgeCall();
    void populatePurgeIds(MojObject& ids);

    /* Callout for loading content from MojoDB in through the Proxy,
     * and later deleting bad data. */
    std::shared_ptr<LunaCall> m_call;

    /* Track old Activities that should be purged */
    typedef std::list<std::shared_ptr<PersistTokenDB> > TokenQueue;
    TokenQueue m_oldTokens;

    DB8ManagerListener* m_listener;
};

#endif /* _DB_MANAGER_H_ */
