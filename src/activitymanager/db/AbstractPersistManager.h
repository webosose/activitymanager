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

#ifndef __PERSIST_PROXY_H__
#define __PERSIST_PROXY_H__

#include <db/ICompletion.h>
#include "Main.h"
#include "activity/Activity.h"
#include "db/AbstractPersistCommand.h"
#include "db/PersistToken.h"

class AbstractPersistManager {
public:
    AbstractPersistManager();
    virtual ~AbstractPersistManager();

    typedef enum CommmandType_e {
        StoreCommandType,
        DeleteCommandType,
        NoopCommandType
    } CommandType;

    virtual std::shared_ptr<AbstractPersistCommand> prepareCommand(CommandType type,
                                                                   std::shared_ptr<Activity> activity,
                                                                   std::shared_ptr<ICompletion> completion);

    virtual std::shared_ptr<AbstractPersistCommand> prepareStoreCommand(
            std::shared_ptr<Activity> activity, std::shared_ptr<ICompletion> completion) = 0;
    virtual std::shared_ptr<AbstractPersistCommand> prepareDeleteCommand(
            std::shared_ptr<Activity> activity, std::shared_ptr<ICompletion> completion) = 0;
    virtual std::shared_ptr<AbstractPersistCommand> prepareNoopCommand(
            std::shared_ptr<Activity> activity, std::shared_ptr<ICompletion> completion);

    virtual std::shared_ptr<PersistToken> createToken() = 0;

    virtual void loadActivities() = 0;

protected:
    static MojLogger s_log;
};

#endif /* __PERSIST_PROXY_H__ */
