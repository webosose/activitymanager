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

#include <db/AbstractPersistManager.h>
#include <stdexcept>

#include "util/Logging.h"

MojLogger AbstractPersistManager::s_log(_T("activitymanager.persistproxy"));

AbstractPersistManager::AbstractPersistManager()
{
}

AbstractPersistManager::~AbstractPersistManager()
{
}

std::shared_ptr<AbstractPersistCommand> AbstractPersistManager::prepareCommand(CommandType type,
                                                                     std::shared_ptr<Activity> activity,
                                                                     std::shared_ptr<ICompletion> completion)
{
    switch (type) {
        case StoreCommandType:
            return prepareStoreCommand(activity, completion);
        case DeleteCommandType:
            return prepareDeleteCommand(activity, completion);
        case NoopCommandType:
            return prepareNoopCommand(activity, completion);
    }

    throw std::runtime_error("Attempt to create command of unknown type");
}

std::shared_ptr<AbstractPersistCommand> AbstractPersistManager::prepareNoopCommand(std::shared_ptr<Activity> activity,
                                                                         std::shared_ptr<ICompletion> completion)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Preparing noop command for [Activity %llu]", activity->getId());

    return std::make_shared<NoopCommand>(activity, completion);
}

