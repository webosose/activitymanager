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

#ifndef __ABSTRACT_PERSIST_COMMAND_H__
#define __ABSTRACT_PERSIST_COMMAND_H__

#include <db/ICompletion.h>
#include "Main.h"

class Activity;

/*
 * Generic command that, when Persist() is called, will attempt to perform
 * a modification to the persistence store, depending on the implementation
 * of the command.
 *
 * Persist() should not throw, and is responsible for *ensuring* Complete()
 * will eventually be called (the second really implies the first), since
 * commands can be chained in causal order.
 *
 * Complete() is a template method - it will execute the completion and
 * pass a result indicating the success or failure of the command.  It
 * will unhook the command from its parent Activity (if that Activity is
 * still valid), and call the next command in the chain.
 *
 * Failure of a command is considered a serious error, and will result in
 * inconsistent state.
 */
class AbstractPersistCommand: public std::enable_shared_from_this<AbstractPersistCommand> {
public:
    AbstractPersistCommand(std::shared_ptr<Activity> activity, std::shared_ptr<ICompletion> completion);

    virtual ~AbstractPersistCommand();

    std::string getString() const;

    std::shared_ptr<Activity> getActivity();

    void append(std::shared_ptr<AbstractPersistCommand> command);

    /* Subclass should override this to issue the persistence operation.
     * The PeristToken of the Activity should be retrieved at the point the
     * command is issued (as opposed to when the command was created) because
     * it may have been updated (or removed) by preceeding PersistCommands. */
    virtual void persist() = 0;

protected:
    virtual std::string getMethod() const = 0;
    /* Specific subclass command implementation should call Complete once
     * the command has finished.  Non-virtual because it shouldn't be
     * overridden. */
    void complete(bool success);
    void validate(bool checkTokenValid) const;

protected:
    std::shared_ptr<Activity> m_activity;
    std::shared_ptr<ICompletion> m_completion;

    std::shared_ptr<AbstractPersistCommand> m_next;

    static MojLogger s_log;
};

class NoopCommand: public AbstractPersistCommand {
public:
    NoopCommand(std::shared_ptr<Activity> activity, std::shared_ptr<ICompletion> completion);
    virtual ~NoopCommand();

    virtual void persist();

protected:
    virtual std::string getMethod() const;
};

#endif /* __ABSTRACT_PERSIST_COMMAND_H__ */
