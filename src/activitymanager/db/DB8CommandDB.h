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

#ifndef __PERSIST_COMMAND_DB_H__
#define __PERSIST_COMMAND_DB_H__

#include <db/DB8Command.h>
#include "activity/Activity.h"

class DB8StoreCommand: public DB8Command {
public:
    DB8StoreCommand(std::shared_ptr<Activity> activity,
                          std::shared_ptr<ICompletion> completion);
    virtual ~DB8StoreCommand();

protected:
    virtual std::string getMethod() const;

    virtual void updateParams(MojObject& params);
    virtual void persistResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);
};

class DB8DeleteCommand: public DB8Command {
public:
    DB8DeleteCommand(std::shared_ptr<Activity> activity,
                           std::shared_ptr<ICompletion> completion);
    virtual ~DB8DeleteCommand();

protected:
    virtual std::string getMethod() const;

    virtual void updateParams(MojObject& params);
    virtual void persistResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);
};

#endif /* __MOJO_DB_PERSIST_COMMAND_H__ */
