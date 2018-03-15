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

#include "db/AbstractPersistCommand.h"

#include <stdexcept>

#include "activity/Activity.h"
#include "db/PersistToken.h"
#include "util/Logging.h"

MojLogger AbstractPersistCommand::s_log(_T("activitymanager.persistcommand"));

AbstractPersistCommand::AbstractPersistCommand(std::shared_ptr<Activity> activity,
                                               std::shared_ptr<ICompletion> completion)
    : m_activity(activity)
    , m_completion(completion)
{
}

AbstractPersistCommand::~AbstractPersistCommand()
{
}

std::shared_ptr<Activity> AbstractPersistCommand::getActivity()
{
    return m_activity;
}

std::string AbstractPersistCommand::getString() const
{
    if (m_activity->isPersistTokenSet()) {
        return (getMethod() + " " + m_activity->getPersistToken()->getString());
    } else {
        return getMethod() + "(no token)";
    }
}

/* Append the new command to the end of the command chain that this
 * command is a member of. */
void AbstractPersistCommand::append(std::shared_ptr<AbstractPersistCommand> command)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    if (command == shared_from_this()) {
        LOG_AM_WARNING(MSGID_APPEND_CMD_TOSELF, 1,
                       PMLOGKS("persist_command", getString().c_str()),
                       "Attempt to append command directly to itself");
        throw std::runtime_error("Attempt to append Persist Command directly to itself");
    }

    std::shared_ptr<AbstractPersistCommand> target;

    for (target = shared_from_this(); target->m_next ; target = target->m_next) {
        if (target->m_next == shared_from_this()) {
            LOG_AM_WARNING(MSGID_APPEND_CREATE_LOOP, 2,
                           PMLOGKS("persist_command", command->getString().c_str()),
                           PMLOGKS("persist_command", target->getString().c_str()),
                           "Append Failed");
            throw std::runtime_error("Attempt to append Persist Command would create a loop");
        }
    }

    target->m_next = command;
}

void AbstractPersistCommand::complete(bool success)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    /* All steps of Complete must execute, including launching the next
     * command in the chain.  All exceptions must be handled locally. */
    try {
        m_completion->complete(success);
    } catch (const std::exception& except) {
        LOG_AM_WARNING(MSGID_CMD_COMPLETE_EXCEPTION, 3,
                       PMLOGKFV("activity", "%llu" ,m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       PMLOGKS("exception", except.what()),
                       "Unexpected exception while trying to complete");
    } catch (...) {
        LOG_AM_WARNING(MSGID_CMD_COMPLETE_EXCEPTION, 2,
                       PMLOGKFV("activity", "%llu", m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       "exception while trying to complete");

    }

    /* Tricky order here.  Unhooking this command will probably destroy it,
     * so we have to pick up a local reference to the next command in the
     * chain because we won't be able to access the object property.
     *
     * The whole chain shouldn't collapse, though, as the next executing
     * command should be referenced by the same Activity or some other one.
     * It's just the reference that's bad... but since something like:
     * Complete "foo"/Create(and replace) "foo"/Cancel old "foo".  If the
     * Complete is still in process, the Cancel will chain (and ultimately
     * will succeed as the Activity is already cancelled).
     */
    std::shared_ptr<AbstractPersistCommand> next = m_next;

    try {
        m_activity->unhookPersistCommand(shared_from_this());
    } catch (const std::exception& except) {
        LOG_AM_WARNING(MSGID_CMD_UNHOOK_EXCEPTION, 3,
                       PMLOGKFV("activity", "%llu", m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       PMLOGKS("exception", except.what()),
                       "Unexpected exception unhooking command");
    } catch (...) {
        LOG_AM_WARNING(MSGID_CMD_UNHOOK_EXCEPTION, 2,
                       PMLOGKFV("activity", "%llu", m_activity->getId()),
                       PMLOGKS("persist_command", getString().c_str()),
                       "exception unhooking command");
    }

    if (next) {
        next->persist();
    }
}

void AbstractPersistCommand::validate(bool checkTokenValid) const
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (!m_activity->isPersistTokenSet()) {
        LOG_AM_ERROR(MSGID_PERSIST_TOKEN_NOT_SET, 2,
                     PMLOGKFV("activity", "%llu", m_activity->getId()),
                     PMLOGKS("persist_command", getString().c_str()),
                     "Persist token for Activity is not set");

        throw std::runtime_error("Persist token for Activity is not set");
    }

    if (!checkTokenValid) {
        return;
    }

    std::shared_ptr<PersistToken> pt = m_activity->getPersistToken();
    if (!pt->isValid()) {
        LOG_AM_ERROR(MSGID_PERSIST_TOKEN_INVALID, 2,
                     PMLOGKFV("activity", "%llu", m_activity->getId()),
                     PMLOGKS("persist_command", getString().c_str()),
                     "Persist token for Activity is set but not valid");

        throw std::runtime_error("Persist token for Activity is set but not valid");
    }
}

NoopCommand::NoopCommand(std::shared_ptr<Activity> activity,
                         std::shared_ptr<ICompletion> completion)
    : AbstractPersistCommand(activity, completion)
{
}

NoopCommand::~NoopCommand()
{
}

void NoopCommand::persist()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [PersistCommand %s]: No-op command",
                 m_activity->getId(), getString().c_str());
    complete(true);
}

std::string NoopCommand::getMethod() const
{
    return "Noop";
}

