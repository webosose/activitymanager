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

#ifndef __PERSIST_COMMAND_H__
#define __PERSIST_COMMAND_H__

#include <core/MojService.h>
#include <db/ICompletion.h>

#include "activity/Activity.h"
#include "base/LunaCall.h"
#include "base/LunaURL.h"
#include "db/AbstractPersistCommand.h"

class DB8Command: public AbstractPersistCommand {
public:
    DB8Command(const LunaURL& method,
                   std::shared_ptr<Activity> activity,
                   std::shared_ptr<ICompletion> completion);

    virtual ~DB8Command();

    virtual void persist();

protected:
    std::string getIdString() const;

    virtual void updateParams(MojObject& params) = 0;
    virtual void persistResponse(MojServiceMessage *msg, const MojObject& response, MojErr err);

    LunaURL m_method;
    MojObject m_params;

    std::shared_ptr<LunaCall> m_call;
};

#endif /* __PERSIST_COMMAND_H__ */
