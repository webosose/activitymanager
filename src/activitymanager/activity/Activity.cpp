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

#include <activity/type/PowerManager.h>
#include "Activity.h"

#include <stdexcept>
#include <algorithm>
#include <glib.h>

#include "state/AbstractActivityState.h"
#include "state/ActivityStateNone.h"
#include "activity/ActivityManager.h"
#include "activity/requirement/ProxyRequirement.h"
#include "conf/ActivityJson.h"
#include "conf/Config.h"
#include "util/Logging.h"

#include "tools/ActivityMonitor.h"

Activity::Activity(activityId_t id)
    : m_id(id)
    , m_persistent(false)
    , m_explicit(false)
    , m_immediate(false)
    , m_priority(ActivityBackgroundPriority)
    , m_useSimpleType(true) /* Background */
    , m_continuous(false)
    , m_userInitiated(false)
    , m_powerDebounce(false)
    , m_scheduled(false)
    , m_ready(false)
    , m_running(false)
    , m_ending(false)
    , m_terminate(false)
    , m_restart(false)
    , m_requeue(false)
    , m_yielding(false)
    , m_released(true)
    , m_state(ActivityStateNone::newInstance())
    , m_triggerMode(TriggerConditionType::kMultiTriggerNone)
    , m_restartTime(0.0)
    , m_restartCount(0)
{
}

Activity::~Activity()
{
    LOG_AM_DEBUG("[Activity %llu] Cleaning up", m_id);
}

activityId_t Activity::getId() const
{
    return m_id;
}

void Activity::setName(const std::string& name)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    m_name = name;
}

const std::string& Activity::getName() const
{
    return m_name;
}

bool Activity::isNameRegistered() const
{
    return m_nameTableItem.is_linked();
}

void Activity::setDescription(const std::string& description)
{
    m_description = description;
}

const std::string& Activity::getDescription() const
{
    return m_description;
}

void Activity::setCreator(const Subscriber& creator)
{
    m_creator = creator;
}

void Activity::setCreator(const BusId& creator)
{
    m_creator = creator;
}

const BusId& Activity::getCreator() const
{
    return m_creator;
}

void Activity::setImmediate(bool immediate)
{
    m_immediate = immediate;
}

bool Activity::isImmediate() const
{
    return m_immediate;
}

MojErr Activity::addSubscription(std::shared_ptr<AbstractSubscription> sub)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] %s subscribed", m_id,
                 sub->getSubscriber().getString().c_str());

    m_subscriptions.insert(*sub);

    SubscriberSet::const_iterator exists = m_subscribers.find(sub->getSubscriber());
    if (exists != m_subscribers.end()) {
        m_subscribers.insert(exists, sub->getSubscriber());
    } else {
        m_subscribers.insert(sub->getSubscriber());
    }

    return MojErrNone;
}

MojErr Activity::removeSubscription(std::shared_ptr<AbstractSubscription> sub)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] %s unsubscribed", m_id,
                 sub->getSubscriber().getString().c_str());

    SubscriptionSet::iterator subscription = m_subscriptions.begin();

    for (; subscription != m_subscriptions.end() ; ++subscription) {
        if (&(*sub) == &(*subscription))
            break;
    }

    if (subscription != m_subscriptions.end()) {
        m_subscriptions.erase(subscription);
    } else {
        LOG_AM_WARNING(
                MSGID_SUBSCRIBER_NOT_FOUND, 2,
                PMLOGKFV("activity", "%llu", m_id),
                PMLOGKS("subscriber",sub->getSubscriber().getString().c_str()),
                "activity unable to find subscriber on the subscriptions list");
        return MojErrInvalidArg;
    }

    Subscriber& actor = sub->getSubscriber();

    SubscriberSet::iterator subscriber = m_subscribers.begin();
    for (; subscriber != m_subscribers.end() ; ++subscriber) {
        if (&actor == &(*subscriber))
            break;
    }

    if (subscriber != m_subscribers.end()) {
        m_subscribers.erase(subscriber);
    } else {
        LOG_AM_DEBUG(
                "[Activity %llu] Unable to find %s's subscriber entry on the subscribers list",
                m_id, actor.getString().c_str());
    }

    AdopterQueue::iterator adopter = m_adopters.begin();
    for (; adopter != m_adopters.end() ; ++adopter) {
        if (sub == adopter->lock())
            break;
    }

    if (adopter != m_adopters.end()) {
        m_adopters.erase(adopter);
    }

    if (m_releasedParent.lock() == sub) {
        m_releasedParent.reset();
    }

    if (m_parent.lock() == sub) {
        m_parent.reset();

        if (m_adopters.empty()) {
            /* If there are no remaining subscribers, just let Abandoned clean
             * up and call EndActivity. */
            if (!m_subscriptions.empty()) {
                orphaned();
            }
        } else {
            m_parent = m_adopters.front().lock();
            m_adopters.pop_front();

            m_parent.lock()->queueEvent(kActivityOrphanEvent);
        }
    }

    if (m_subscriptions.empty()) {
        abandoned();
    }

    return MojErrNone;
}

bool Activity::isSubscribed() const
{
    return !m_subscriptions.empty();
}

MojErr Activity::broadcastEvent(ActivityEvent event)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Broadcasting \"%s\" event", m_id,
                 ActivityEventNames[event]);

    MojErr err = MojErrNone;

    for (SubscriptionSet::iterator iter = m_subscriptions.begin() ;
            iter != m_subscriptions.end() ; ++iter) {
        MojErr sendErr = iter->queueEvent(event);

        /* Catch the last error, if any */
        if (sendErr)
            err = sendErr;
    }

    if (err != MojErrNone)
        LOG_AM_DEBUG("[Activity %llu] catch the last error %d", m_id, (int)err);

    return MojErrNone;
}

void Activity::plugAllSubscriptions()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Plugging all subscriptions", m_id);

    for (SubscriptionSet::iterator iter = m_subscriptions.begin() ;
            iter != m_subscriptions.end() ; ++iter) {
        iter->plug();
    }
}

void Activity::unplugAllSubscriptions()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Unplugging all subscriptions", m_id);

    for (SubscriptionSet::iterator iter = m_subscriptions.begin() ;
            iter != m_subscriptions.end() ; ++iter) {
        iter->unplug();
    }
}

void Activity::setTriggerVector(std::vector<std::shared_ptr<ITrigger>> vec)
{
    std::set<std::string> set;
    std::vector<std::shared_ptr<ITrigger>> unnamed;

    for (std::shared_ptr<ITrigger>& trigger : vec) {
        if (trigger->getName().empty()) {
            unnamed.push_back(trigger);
        } else {
            auto it = set.find(trigger->getName());
            if (it == set.end()) {
                set.insert(trigger->getName());
            } else {
                throw std::runtime_error("duplicate trigger name");
            }
        }
    }

    const std::string baseName = "trigger";
    int count = 0;
    std::string name;
    for (std::shared_ptr<ITrigger>& trigger : unnamed) {
        do {
            count++;
            name = baseName + std::to_string(count);
        } while (set.find(name) != set.end());
        trigger->setName(name);
    }

    m_triggerVector = vec;
}

std::vector<std::shared_ptr<ITrigger>> Activity::getTriggers() const
{
    return m_triggerVector;
}

void Activity::clearTrigger()
{
    m_triggerVector.clear();
}

bool Activity::hasTrigger() const
{
    return !m_triggerVector.empty();
}

void Activity::setMessageForStart(MojServiceMessage *msg)
{
    m_msgForStart = MojRefCountedPtr<MojServiceMessage>(msg);
}

TriggerConditionType Activity::getTriggerMode()
{
    return m_triggerMode;
}

void Activity::setTriggerMode(TriggerConditionType mode)
{
    m_triggerMode = mode;
}

std::shared_ptr<ITrigger> Activity::getTrigger(std::string name)
{
    auto it = std::find_if(
            m_triggerVector.begin(),
            m_triggerVector.end(),
            [&] (const std::shared_ptr<ITrigger> trigger) { return trigger->getName() == name; });
    if (it == m_triggerVector.end()) {
        return nullptr;
    } else {
        return (*it);
    }
}

void Activity::onFailTrigger(std::shared_ptr<ITrigger> trigger)
{
    LOG_AM_DEBUG("[Activity %llu] Trigger \"%s\" Fail", m_id, trigger->getName().c_str());

    respondForStart("Failed to subscribe trigger: " + trigger->getName());

    m_state->onTriggerFail(shared_from_this(), trigger);
}

void Activity::onSuccessTrigger(std::shared_ptr<ITrigger> trigger, bool statusChanged, bool valueChanged)
{
    LOG_AM_DEBUG("[Activity %llu] Trigger \"%s\" %s", m_id, trigger->getName().c_str(),
                 trigger->isSatisfied() ? "Satisfiy" : "Unsatisfy");

    respondForStart();

    if (statusChanged) {
        m_state->onTriggerUpdate(shared_from_this(), trigger);
    }
}

bool Activity::isTriggered() const
{
    if (m_triggerVector.empty()) {
        // return true for empty trigger to handle some activity, which does not have trigger
        // example : Activity have time-based requirements only.
        return true;
    }

    for (auto& trigger : m_triggerVector) {
        switch (m_triggerMode) {
        case TriggerConditionType::kMultiTriggerNone:
        case TriggerConditionType::kMultiTriggerOrMode:
            if (trigger->isSatisfied()) {
                return true;
            }
            break;

        case TriggerConditionType::kMultiTriggerAndMode:
            if (!trigger->isSatisfied()) {
                return false;
            }
            break;
        }
    }
    return (m_triggerMode == TriggerConditionType::kMultiTriggerAndMode) ? true : false;
}

void Activity::setCallback(std::shared_ptr<AbstractCallback> callback)
{
    m_callback = callback;
}

std::shared_ptr<AbstractCallback> Activity::getCallback()
{
    return m_callback;
}

bool Activity::hasCallback() const
{
    return m_callback != NULL;
}

void Activity::callbackFailed(std::shared_ptr<AbstractCallback> callback,
                              AbstractCallback::FailureType failure)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Callback failed", m_id);

    if (AbstractCallback::FailureType::PermanentFailure == failure) {
        m_state->onCallbackFail(shared_from_this(), callback);
        return;
    }

    LOG_AM_DEBUG("[Activity %llu] Callback failed, but consider as success", getId());
    callbackSucceeded(callback);
}

void Activity::callbackSucceeded(std::shared_ptr<AbstractCallback> callback)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Callback succeeded", m_id);

    m_state->onCallbackSucceed(shared_from_this(), callback);
}

void Activity::setSchedule(std::shared_ptr<Schedule> schedule)
{
    m_schedule = schedule;
}

void Activity::clearSchedule()
{
    m_schedule.reset();
}

void Activity::scheduled()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Scheduled", m_id);

    m_state->onSchedule(shared_from_this(), m_schedule);
}

bool Activity::isScheduled() const
{
    if (m_schedule) {
        return m_schedule->isScheduled();
    } else {
        return true;
    }
}

void Activity::addRequirement(std::shared_ptr<IRequirement> requirement)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] setting [Requirement %s]", m_id,
                 requirement->getName().c_str());

    auto req = std::make_shared<ProxyRequirement>(requirement);
    m_requirements[requirement->getName()] = req;
    req->addListener(shared_from_this());
}

void Activity::removeRequirement(const std::string& name)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] removing [Requirement %s] by name", m_id, name.c_str());

    auto found = m_requirements.find(name);
    if (found == m_requirements.end()) {
        LOG_AM_WARNING(MSGID_RM_REQ_NOT_FOUND, 2,
                       PMLOGKFV("Activity", "%llu", m_id),
                       PMLOGKS("Requirement", name.c_str()),
                       "not found while trying to remove by name");
        return;
    }

    auto req = found->second;
    req->removeListener(shared_from_this());
    m_requirements.erase(found);
}

bool Activity::isRequirementSet(const std::string& name) const
{
    auto found = m_requirements.find(name);
    return (found != m_requirements.end());
}

bool Activity::hasRequirements() const
{
    return (!m_requirements.empty());
}

bool Activity::isRequirementMet() const
{
    for (auto it = m_requirements.begin(); it != m_requirements.end(); ++it) {
        if (!(*it).second->isMet())
            return false;
    }

    return true;
}

std::shared_ptr<IRequirement> Activity::getRequirement(std::string name)
{
    auto it = m_requirements.find(name);
    if (it == m_requirements.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

void Activity::onMetRequirement(std::string name)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [Requirement %s] met", m_id, name.c_str());

    auto it = m_requirements.find(name);
    if (it == m_requirements.end()) {
        LOG_AM_ERROR(MSGID_REQ_MET_OWNER_MISMATCH, 2,
                     PMLOGKS("Requirement", name.c_str()),
                     PMLOGKFV("Activity", "%llu", m_id),
                     "Met requirement is not owned by activity");
        throw std::runtime_error("Requirement owner mismatch");
    }

    m_state->onRequirementUpdate(shared_from_this(), (*it).second);
}

void Activity::onUnmetRequirement(std::string name)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [Requirement %s] unmet", m_id, name.c_str());

    auto it = m_requirements.find(name);
    if (it == m_requirements.end()) {
        LOG_AM_ERROR(MSGID_REQ_UNMET_OWNER_MISMATCH, 2,
                     PMLOGKFV("Activity", "%llu", m_id),
                     PMLOGKS("Requirement", name.c_str()),
                     "Unmet requirement is not owned by activity");
        throw std::runtime_error("Requirement owner mismatch");
    }

    m_state->onRequirementUpdate(shared_from_this(), (*it).second);
}

void Activity::onUpdateRequirement(std::string name)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] [Requirement %s] updated", m_id, name.c_str());

    auto it = m_requirements.find(name);
    if (it == m_requirements.end()) {
        LOG_AM_ERROR(MSGID_UNOWNED_UPDATED_REQUIREMENT, 2,
                     PMLOGKFV("Activity", "%llu", m_id),
                     PMLOGKS("Requirement", name.c_str()),
                     "Updated requirement is not owned by activity");
        throw std::runtime_error("Requirement owner mismatch");
    }

    m_state->onRequirementUpdate(shared_from_this(), (*it).second);
}

void Activity::setParent(std::shared_ptr<AbstractSubscription> sub)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Attempting to set %s as parent", m_id,
                 sub->getSubscriber().getString().c_str());

    if (!m_parent.expired())
        throw std::runtime_error("Activity already has parent set");

    m_parent = sub;
    m_released = false;

    LOG_AM_DEBUG("[Activity %llu] %s assigned as parent", m_id,
                 sub->getSubscriber().getString().c_str());
}

BusId Activity::getParent() const
{
    if  (m_parent.expired()) {
        return BusId();
    }

    return m_parent.lock()->getSubscriber();
}

bool Activity::hasParent() const
{
    return !m_parent.expired();
}

MojErr Activity::adopt(std::shared_ptr<AbstractSubscription> sub, bool wait, bool *adopted)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] %s attempting to adopt %s", m_id,
                 sub->getSubscriber().getString().c_str(),
                 wait ? "and willing to wait" : "");

    m_adopters.push_back(sub);

    if (!m_parent.expired()) {
        if (adopted)
            *adopted = false;

        if (!wait) {
            LOG_AM_DEBUG("[Activity %llu] already has a parent (%s), and %s does not want to wait",
                         m_id, m_parent.lock()->getSubscriber().getString().c_str(),
                         sub->getSubscriber().getString().c_str());

            return MojErrWouldBlock;
        }

        LOG_AM_DEBUG("[Activity %llu] %s added to adopter list", m_id,
                     sub->getSubscriber().getString().c_str());
    } else {
        if (adopted)
            *adopted = true;

        doAdopt();
    }

    return MojErrNone;
}

MojErr Activity::release(const BusId& caller)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] %s attempting to release", m_id,
                 caller.getString().c_str());

    /* Can't release if it's already been released. */
    if (m_released) {
        LOG_AM_DEBUG("[Activity %llu] Has already been released", m_id);
        return MojErrAccessDenied;
    }

    if (m_parent.lock()->getSubscriber() != caller) {
        LOG_AM_DEBUG("[Activity %llu] %s failed to release, as %s is currently the parent",
                     m_id, caller.getString().c_str(),
                     m_parent.lock()->getSubscriber().getString().c_str());
        return MojErrAccessDenied;
    }

    m_releasedParent = m_parent;
    m_parent.reset();
    m_released = true;

    LOG_AM_DEBUG("[Activity %llu] Released by %s", m_id,
                 caller.getString().c_str());

    if (!m_adopters.empty()) {
        doAdopt();
    }

    return MojErrNone;
}

bool Activity::isReleased() const
{
    return m_released;
}

MojErr Activity::complete(const BusId& caller, bool force)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] %s attempting to complete", m_id,
                 caller.getString().c_str());

    if (!force &&
            (m_released || m_parent.expired() ||
                    (m_parent.lock()->getSubscriber() != caller)) &&
            (caller != m_creator))
        return MojErrAccessDenied;

    m_state->complete(shared_from_this());

    LOG_AM_DEBUG("[Activity %llu] Completed by %s", m_id,
                 caller.getString().c_str());

    return MojErrNone;
}

void Activity::setPersistent(bool persistent)
{
    m_persistent = persistent;
}

bool Activity::isPersistent() const
{
    return m_persistent;
}

void Activity::setPersistToken(std::shared_ptr<PersistToken> token)
{
    m_persistToken = token;
}

std::shared_ptr<PersistToken> Activity::getPersistToken()
{
    return m_persistToken;
}

void Activity::clearPersistToken()
{
    m_persistToken.reset();
}

bool Activity::isPersistTokenSet() const
{
    if (m_persistToken)
        return true;
    else
        return false;
}

void Activity::hookPersistCommand(std::shared_ptr<AbstractPersistCommand> cmd)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Hooking [PersistCommand %s]", m_id,
                 cmd->getString().c_str());

    if (cmd->getActivity()->getId() != m_id) {
        LOG_AM_ERROR(MSGID_HOOK_PERSIST_CMD_ERR,
                     3,
                     PMLOGKS("persist_command", cmd->getString().c_str()),
                     PMLOGKFV("Assigned_activity","%llu", cmd->getActivity()->getId()),
                     PMLOGKFV(" Current activity","%llu" ,m_id),
                     "Attempt to hook persist command assigned to a different Activity");
        throw std::runtime_error(
                "Attempt to hook persist command which is currently assigned to a different Activity");
    }

    m_persistCommands.push_back(cmd);
}

void Activity::unhookPersistCommand(std::shared_ptr<AbstractPersistCommand> cmd)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Unhooking [PersistCommand %s]", m_id,
                 cmd->getString().c_str());

    if (cmd->getActivity()->getId() != m_id) {
        LOG_AM_ERROR(MSGID_UNHOOK_CMD_ACTVTY_ERR, 3,
                     PMLOGKFV("activity", "%llu", m_id),
                     PMLOGKS("persist_command", cmd->getString().c_str()),
                     PMLOGKFV("Assigned_activity", "%llu",cmd->getActivity()->getId()),
                     "Attempt to unhook PersistCommand assigned to different activity");
        throw std::runtime_error(
                "Attempt to unhook persist command which is currently assigned to a different Activity");
    }

    if (cmd != m_persistCommands.front()) {
        CommandQueue::iterator found = std::find(m_persistCommands.begin(),
                                                 m_persistCommands.end(),
                                                 cmd);
        if (found != m_persistCommands.end()) {
            LOG_AM_WARNING(MSGID_UNHOOK_CMD_QUEUE_ORDERING_ERR, 2,
                           PMLOGKFV("Activity","%llu",m_id),
                           PMLOGKS("PersistCommand",cmd->getString().c_str()),
                           "Request to unhook persistCommand which is not the first persist command in the queue");
            m_persistCommands.erase(found);
        } else {
            LOG_AM_WARNING(MSGID_UNHOOK_CMD_NOT_IN_QUEUE, 2,
                           PMLOGKFV("Activity","%llu",m_id),
                           PMLOGKS("PersistCommand",cmd->getString().c_str()),
                           "PersistCommand Not in the queue");
        }
    } else {
        m_persistCommands.pop_front();

        /* If any subscriptions were holding events... */
        if (m_persistCommands.empty()) {
            unplugAllSubscriptions();
        }
    }

    if (m_ending && m_persistCommands.empty()) {
        endActivity();
    }
}

bool Activity::isPersistCommandHooked() const
{
    return (!m_persistCommands.empty());
}

std::shared_ptr<AbstractPersistCommand> Activity::getHookedPersistCommand()
{
    if (m_persistCommands.empty()) {
        LOG_AM_ERROR(MSGID_HOOKED_PERSIST_CMD_NOT_FOUND, 1,
                     PMLOGKFV("activity", "%llu", m_id),
                     "Attempt to retreive the current hooked persist command, but none is present");
        throw std::runtime_error(
                "Attempt to retreive hooked persist command, but none is present");
    }

    return m_persistCommands.front();
}

void Activity::setPriority(ActivityPriority_t priority)
{
    m_priority = priority;
}

ActivityPriority_t Activity::getPriority() const
{
    return m_priority;
}

void Activity::setExplicit(bool explicitActivity)
{
    m_explicit = explicitActivity;
}

bool Activity::isExplicit() const
{
    return m_explicit;
}

void Activity::setTerminateFlag(bool terminate)
{
    m_terminate = terminate;
}

void Activity::setRestartFlag(bool restart)
{
    m_restart = restart;
}

void Activity::setUseSimpleType(bool useSimpleType)
{
    m_useSimpleType = useSimpleType;
}

void Activity::setContinuous(bool continuous)
{
    m_continuous = continuous;
}

bool Activity::isContinuous() const
{
    return m_continuous;
}

void Activity::setUserInitiated(bool userInitiated)
{
    m_userInitiated = userInitiated;
}

bool Activity::isUserInitiated() const
{
    return m_userInitiated;
}

void Activity::setPowerActivity(std::shared_ptr<AbstractPowerActivity> powerActivity)
{
    m_powerActivity = powerActivity;
}

std::shared_ptr<AbstractPowerActivity> Activity::getPowerActivity()
{
    return m_powerActivity;
}

bool Activity::isPowerActivity() const
{
    return m_powerActivity != NULL;
}

void Activity::powerLockedNotification()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG(
            "[Activity %llu] Received notification that power has been successfully locked on",
            m_id);

    PowerManager::getInstance().confirmPowerActivityBegin(shared_from_this());

    if (!m_ending) {
        doRunActivity();
    }
}

void Activity::powerUnlockedNotification()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG(
            "[Activity %llu] Received notification that power has been successfully unlocked",
            m_id);

    PowerManager::getInstance().confirmPowerActivityEnd(shared_from_this());

    if (m_ending) {
        endActivity();
    }
}

void Activity::setPowerDebounce(bool powerDebounce)
{
    m_powerDebounce = powerDebounce;
}

bool Activity::isPowerDebounce() const
{
    return m_powerDebounce;
}

void Activity::setMetadata(const std::string& metadata)
{
    m_metadata = metadata;
}

void Activity::clearMetadata()
{
    m_metadata.clear();
}

void Activity::restartActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Restarting", m_id);

    m_ready = false;
    m_running = false;
    m_ending = false;
    m_restart = false;
    m_requeue = false;
    m_yielding = false;
}

void Activity::requestScheduleActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Requesting permission to schedule from Activity Manager", m_id);

    ActivityManager::getInstance().informActivityInitialized(shared_from_this());
}

void Activity::respondForStart(std::string errorText)
{
    if (!m_msgForStart.get()) {
        return;
    }

    if (errorText.length() > 0) {
        (void) m_msgForStart->replyError(MojErrInternal, errorText.c_str());
        m_msgForStart.reset();
        return;
    }

    if (!hasTrigger()) {
        goto Respond;
    }

    if (m_triggerMode == kMultiTriggerAndMode) {
        for (auto& trigger : m_triggerVector) {
            if (trigger->getSubscriptionCount() <= 0) {
                return;
            }
        }
    } else {
        for (auto& trigger : m_triggerVector) {
            if (trigger->getSubscriptionCount()) {
                goto Respond;
            }
        }
        return;
    }

Respond:
    MojErr errs = MojErrNone;
    MojErr err = MojErrNone;
    MojObject reply;

    err = reply.putInt(_T("activityId"), (MojInt64) m_id);
    MojErrAccumulate(errs, err);

    err = reply.putBool(_T("subscribed"), m_msgForStart->subscribed());
    MojErrAccumulate(errs, err);

    err = m_msgForStart->replySuccess(reply);
    MojErrAccumulate(errs, err);

    m_msgForStart.reset();

    if (MojErrNone != errs) {
        LOG_AM_WARNING(MSGID_ACTVTY_REPLY_ERR, 1,
                       PMLOGKFV("Activity", "%llu", m_id),
                       "Failed to generate to reply Create request");
    }
}

void Activity::scheduleActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Received permission to schedule", m_id);

    if (m_scheduled) {
        respondForStart(); // paused -> start
        return;
    }

    for (auto& trigger : m_triggerVector) {
        trigger->init();
    }

    if (m_schedule) {
        m_schedule->queue();
    }

    m_scheduled = true;
    respondForStart();
}

void Activity::unscheduleActivity()
{
    if (!m_scheduled) {
        return;
    }

    for (auto& trigger : m_triggerVector) {
        trigger->clear();
    }

    if (m_schedule) {
        if (m_schedule->isQueued()) {
            m_schedule->unQueue();
        }
        m_schedule->informActivityFinished();
    }

    m_scheduled = false;

    if (m_msgForStart.get()) {
        (void) m_msgForStart->replyError(MojErrInternal, "Activity cancelled");
        m_msgForStart.reset();
    }
}

void Activity::requestRunActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Requesting permission to run from Activity Manager", m_id);

    m_ready = true;
    ActivityManager::getInstance().informActivityReady(shared_from_this());
}

void Activity::runActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Received permission to run", m_id);

    m_running = true;

    if (m_powerActivity &&
            (m_powerActivity->getPowerState() != AbstractPowerActivity::kPowerLocked)) {
        LOG_AM_DEBUG("[Activity %llu] Requesting power be locked on", m_id);

        /* Request power be locked on, wait to actually start Activity until
         * the power callback informs the Activity this has been accomplished */
        PowerManager::getInstance().requestBeginPowerActivity(shared_from_this());
    } else {
        doRunActivity();
    }
}

void Activity::requestRequeueActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Preparing to requeue", m_id);

    m_requeue = true;

    /* End this particular iteration of the Activity, but start from the
     * scheduled point, rather than all the way at the top */
    endActivity();
}

void Activity::requeueActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Requeuing", m_id);

    m_running = false;
    m_ending = false;
    m_restart = false;
    m_requeue = false;
    m_yielding = false;

    if (isRunnable()) {
        requestRunActivity();
    } else {
        /* Requirements aren't met, or the Activity is paused. */
        m_ready = false;
        ActivityManager::getInstance().informActivityNotReady(shared_from_this());
    }
}

void Activity::yieldActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (m_yielding) {
        LOG_AM_DEBUG("[Activity %llu] Already yielding", m_id);
        return;
    }
    LOG_AM_DEBUG("[Activity %llu] Yielding", m_id);

    m_yielding = true;
    m_requeue = true;

    broadcastEvent(kActivityYieldEvent);
    endActivity();
}

/*
 * If scheduled, remove from the queue.
 */
void Activity::endActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Ending", m_id);

    if (!m_ending) {
        m_ending = true;
        ActivityManager::getInstance().informActivityEnding(shared_from_this());
    }

    bool requeue = shouldRequeue();

    /* If there are no subscriptions, and power unlock is not still pending,
     * check for restart, otherwise unregister. */
    if (!m_subscriptions.empty()) {
        return;
    }

    /* If the power is still locked on, begin unlock and wait for it to
     * complete.  This may result in the function being called
     * immediately from End() if direct power unlock is possible.
     * That's ok.
     *
     * Note: Power unlock shouldn't be initiated until after all the
     * subscribers exit
     */
    if (m_powerActivity &&
            (m_powerActivity->getPowerState() != AbstractPowerActivity::kPowerUnlocked)) {
        PowerManager::getInstance().requestEndPowerActivity(shared_from_this());
    } else if (m_persistCommands.empty()) {
        /* Don't move on to restarting (a potentially updated Activity)
         * until the updates have gone through. */

        /* It's ok for the Activity to restart after this.  Just let the
         * Activity Manager know that that this incarnation of the Activity
         * is done */
        ActivityManager::getInstance().informActivityEnd(shared_from_this());

        if (requeue) {
            requeueActivity();
        } else {
            bool restart = shouldRestart();

            if (restart) {
                restartActivity();
            } else {
                ActivityManager::getInstance().releaseActivity(shared_from_this());
            }
        }
    }
}

/*
 * Any Triggers have fired, Requirements are met, it is Scheduled (and not
 * ending already)
 */
bool Activity::isRunnable() const
{
    if (!isTriggered()) {
        return false;
    }

    if (m_schedule && !m_schedule->isScheduled())
        return false;

    if (!isRequirementMet())
        return false;

    return true;
}

bool Activity::isRunning() const
{
    if (m_running && !m_ending) {
        return true;
    } else {
        return false;
    }
}

bool Activity::isYielding() const
{
    if (m_yielding) {
        return true;
    } else {
        return false;
    }
}

bool Activity::shouldRestart() const
{
    if (m_callback && !m_terminate &&
            (m_restart ||
                m_persistent ||
                m_explicit ||
                (m_schedule && m_schedule->shouldReschedule()))) {
        return true;
    } else {
        return false;
    }
}

bool Activity::shouldRequeue() const
{
    if (m_callback && m_requeue && !m_restart) {
        return true;
    } else {
        return false;
    }
}

void Activity::orphaned()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Orphaned", m_id);

    if (m_ending) {
        /* No particular action... waiting for all subscriptions to cancel */
    } else if (m_running) {
        /* Activity should Cancel.  If persistent, it may re-queue */
        m_state->destroy(shared_from_this());
    } else if (m_scheduled) {
        if (m_callback) {
            /* Activity is not running yet, but its callback will be used
             * (ultimately to aquire a parent) when it does start. */
        } else {
            /* No adopters or way to aquire a new parent, so cancel. */
            m_state->destroy(shared_from_this());
        }
    } else {
        /* Was never fully initialized.  May have subscriptions/subscribers
         * who should be notified, so Cancel. */
        m_state->destroy(shared_from_this());
    }
}

void Activity::abandoned()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Abandoned (no subscribers remaining)", m_id);

    endActivity();
}

void Activity::doAdopt()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Attempting to find new parent", m_id);

    if (m_adopters.empty())
        throw std::runtime_error("No parents available for adoption");

    /* If the old parent is still waiting for notification of the adoption,
     * inform them and drop the reference */
    if (!m_releasedParent.expired()) {
        m_releasedParent.lock()->queueEvent(kActivityAdoptedEvent);
        m_releasedParent.reset();
    }

    m_parent = m_adopters.front();
    m_adopters.pop_front();

    m_parent.lock()->queueEvent(kActivityOrphanEvent);

    m_released = false;
    if (m_ending) {
        LOG_AM_DEBUG("[Activity %llu] Removing ending flag since found new parent", m_id);
    }
    m_ending = false;

    LOG_AM_DEBUG("[Activity %llu] Adopted by %s", m_id,
                 m_parent.lock()->getSubscriber().getString().c_str());
}

void Activity::doRunActivity()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Running", m_id);

    /* Power is now on, if Applicable.  Tell the Activity Manager we're
     * ready to go! */
    ActivityManager::getInstance().informActivityRunning(shared_from_this());

    if (m_callback) {
        doCallback();
    }
}

void Activity::doCallback()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Attempting to generate callback", m_id);

    if (!m_callback)
        throw std::runtime_error("No callback exists for Activity");

    MojErr err = m_callback->call();

    if (err) {
        LOG_AM_ERROR(MSGID_ACTIVITY_CB_FAIL, 1,
                     PMLOGKFV("Activity", "%llu", m_id),
                     "Attempt to call callback failed");
        throw std::runtime_error("Failed to call Activity Callback");
    }
}

void Activity::setState(std::shared_ptr<AbstractActivityState> state)
{
    LOG_AM_DEBUG("[Activity %llu] State \"%s\" -> \"%s\"", m_id,
                 m_state->getName().c_str(), state->getName().c_str());

    m_state = state;
    ActivityMonitor::getInstance().send(shared_from_this());
    g_timeout_add_full(G_PRIORITY_HIGH, 0, &Activity::setStateInternal, &m_id, NULL);
}

gboolean Activity::setStateInternal(gpointer data)
{
    activityId_t* id = static_cast<activityId_t*>(data);
    std::shared_ptr<Activity> self;
    try {
        self = ActivityManager::getInstance().getActivity((*id));
    } catch (const std::runtime_error& except) {
        LOG_AM_WARNING("SET_STATE_ERR", 0, "%s", except.what());
        return G_SOURCE_REMOVE;
    }

    LOG_AM_DEBUG("[Activity %llu] State \"%s\"", self->getId(), self->m_state->getName().c_str());
    self->m_state->enter(self);
    return G_SOURCE_REMOVE;
}

std::shared_ptr<AbstractActivityState> Activity::getState() const
{
    return m_state;
}

bool Activity::isFastRestart()
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    double currentTime = spec.tv_sec + (spec.tv_nsec / 1000000000.0);
    double diff = currentTime - m_restartTime;

    if (diff < Config::getInstance().getRestartLimitInterval()) {
        if (++m_restartCount >= Config::getInstance().getRestartLimitCount()) {
            LOG_AM_WARNING("RESTART_FAST", 3,
                           PMLOGKFV("Activity", "%llu", m_id),
                           PMLOGKFV("interval", "%0.3f", diff),
                           PMLOGKFV("count", "%d", m_restartCount), "");
            return true;
        }
    } else {
        m_restartTime = currentTime;
        m_restartCount = 0;
    }
    return false;
}

pbnjson::JValue Activity::requirementsToJson() const
{
    pbnjson::JValue root = pbnjson::Array();

    if (m_requirements.empty()) {
        return root;
    }

    for (auto& requirement : m_requirements) {
        root.append(requirement.second->toJson());
    }

    return root;
}

pbnjson::JValue Activity::triggerToJson() const
{
    pbnjson::JValue root = pbnjson::Object();

    if (m_triggerVector.empty()) {
        return root;
    }

    pbnjson::JValue triggers = pbnjson::Array();
    for (auto& trigger : m_triggerVector) {
        triggers.append(trigger->toJson());
    }

    if (m_triggerMode == TriggerConditionType::kMultiTriggerOrMode) {
        root.put("or", triggers);
    } else {
        root.put("and", triggers);
    }

    return root;
}

pbnjson::JValue Activity::toJson() const
{
    pbnjson::JValue obj =  pbnjson::JObject{
            {"activityId", (int64_t)m_id},
            {"name", m_name},
            {"state", m_state->toString()},
            {"parent", m_parent.expired() ? "" : m_parent.lock()->getSubscriber().getString().c_str()},
            {"requirements", requirementsToJson()},
            {"trigger", triggerToJson()}};

    return obj;
}

MojErr Activity::identityToJson(MojObject& rep) const
{
    /* Populate a JSON object with the minimal but important identifying
     * information for an Activity - id, name, and creator */

    MojErr errs = MojErrNone;

    MojErr err = rep.putInt(_T("activityId"), (MojInt64) m_id);
    MojErrAccumulate(errs, err);

    err = rep.putString(_T("name"), m_name.c_str());
    MojErrAccumulate(errs, err);

    MojObject creator(MojObject::TypeObject);
    err = m_creator.toJson(creator);
    MojErrAccumulate(errs, err);

    err = rep.put(_T("creator"), creator);
    MojErrAccumulate(errs, err);

    return errs;
}

MojErr Activity::activityInfoToJson(MojObject& rep) const
{
    /* Populate "$activity" property with markup about the Activity.
     * This should include the activityId and the output of the Trigger,
     * if present.
     *
     * XXX it should also include error information if the Activity failed to
     * complete and had to be re-started, or if the Activity's callback failed
     * to result in an Adoption. */

    MojObject callbackRep;
    MojObject triggerRep;
    MojObject requirementsRep;
    MojObject creator;
    MojObject metadataJson;
    MojObject type;
    MojErr err;

    err = rep.putInt(_T("activityId"), (MojInt64) m_id);
    MojErrCheck(err);

    if (hasCallback()) {
        err = m_callback->toJson(callbackRep, ACTIVITY_JSON_CURRENT);
        MojErrCheck(err);

        err = rep.put(_T("callback"), callbackRep);
        MojErrCheck(err);
    }

    if (hasTrigger()) {
        err = triggerToJson(triggerRep, ACTIVITY_JSON_CURRENT);
        MojErrCheck(err);

        err = rep.put(_T("trigger"), triggerRep);
        MojErrCheck(err);
    }

    if (hasRequirements()) {
        err = requirementsToJson(requirementsRep, ACTIVITY_JSON_CURRENT);
        MojErrCheck(err);

        err = rep.put(_T("requirements"), requirementsRep);
        MojErrCheck(err);
    }

    err = m_creator.toJson(creator);
    MojErrCheck(err);

    err = rep.put(_T("creator"), creator);
    MojErrCheck(err);

    err = rep.putString(_T("name"), m_name.c_str());
    MojErrCheck(err);

    if (!m_metadata.empty()) {
        err = metadataJson.fromJson(m_metadata.c_str());
        MojErrCheck(err);

        err = rep.put(_T("metadata"), metadataJson);
        MojErrCheck(err);
    }

    err = type.put("continuous", m_continuous);
    MojErrCheck(err);

    err = rep.put(_T("type"), type);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr Activity::toJson(MojObject& rep, unsigned flags) const
{
    MojObject metadataJson;
    MojObject type;
    MojObject callbackRep;
    MojObject scheduleRep;
    MojObject triggerRep;
    MojObject requirementsRep;
    MojObject internalRep;
    MojErr err = MojErrNone;

    err = rep.putInt(_T("activityId"), (MojInt64) m_id);
    MojErrCheck(err);

    err = rep.putString(_T("name"), m_name.c_str());
    MojErrCheck(err);

    err = rep.putString(_T("description"), m_description.c_str());
    MojErrCheck(err);

    if (!m_metadata.empty()) {
        err = metadataJson.fromJson(m_metadata.c_str());
        MojErrCheck(err);

        err = rep.put(_T("metadata"), metadataJson);
        MojErrCheck(err);
    }

    MojObject creator(MojObject::TypeObject);
    err = m_creator.toJson(creator);
    MojErrCheck(err);

    err = rep.put(_T("creator"), creator);
    MojErrCheck(err);

    if (!(flags & ACTIVITY_JSON_PERSIST)) {
        err = rep.putString(_T("state"), m_state->toString().c_str());
        MojErrCheck(err);

        if (flags & ACTIVITY_JSON_SUBSCRIBERS) {
            if (!m_parent.expired()) {
                MojObject subscriber(MojObject::TypeObject);

                err = m_parent.lock()->getSubscriber().toJson(subscriber);
                MojErrCheck(err);

                err = rep.put(_T("parent"), subscriber);
                MojErrCheck(err);
            }

            MojObject subscriberArray(MojObject::TypeArray);
            for (SubscriptionSet::const_iterator iter =
                    m_subscriptions.begin() ; iter != m_subscriptions.end() ;
                    ++iter) {
                MojObject subscriber(MojObject::TypeObject);

                err = iter->getSubscriber().toJson(subscriber);
                MojErrCheck(err);

                err = subscriberArray.push(subscriber);
                MojErrCheck(err);
            }

            err = rep.put(_T("subscribers"), subscriberArray);
            MojErrCheck(err);

            MojObject adopterArray(MojObject::TypeArray);
            for (AdopterQueue::const_iterator iter = m_adopters.begin() ;
                    iter != m_adopters.end() ; ++iter) {
                MojObject subscriber(MojObject::TypeObject);

                err = iter->lock()->getSubscriber().toJson(subscriber);
                MojErrCheck(err);

                err = adopterArray.push(subscriber);
                MojErrCheck(err);
            }

            err = rep.put(_T("adopters"), adopterArray);
            MojErrCheck(err);
        }
    }

    if (flags & ACTIVITY_JSON_DETAIL) {

        err = typeToJson(type, flags);
        MojErrCheck(err);

        err = rep.put(_T("type"), type);
        MojErrCheck(err);

        if (m_callback) {

            err = m_callback->toJson(callbackRep, flags);
            MojErrCheck(err);

            err = rep.put(_T("callback"), callbackRep);
            MojErrCheck(err);
        }

        if (m_schedule) {

            err = m_schedule->toJson(scheduleRep, flags);
            MojErrCheck(err);

            err = rep.put(_T("schedule"), scheduleRep);
            MojErrCheck(err);
        }

        if (!m_triggerVector.empty()) {
            err = triggerToJson(triggerRep, flags);
            MojErrCheck(err);

            err = rep.put(_T("trigger"), triggerRep);
            MojErrCheck(err);
        }

        if (hasRequirements()) {

            err = requirementsToJson(requirementsRep, flags);
            MojErrCheck(err);

            err = rep.put(_T("requirements"), requirementsRep);
            MojErrCheck(err);
        }
    }

    if (flags & ACTIVITY_JSON_INTERNAL) {

        err = internalToJson(internalRep, flags);
        MojErrCheck(err);

        err = rep.put(_T("internal"), internalRep);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr Activity::typeToJson(MojObject& rep, unsigned flags) const
{
    MojErr err;

    if (m_persistent) {
        err = rep.putBool(_T("persist"), true);
        MojErrCheck(err);
    }

    if (m_explicit) {
        err = rep.putBool(_T("explicit"), true);
        MojErrCheck(err);
    }

    bool useSimpleType = m_useSimpleType;

    if (useSimpleType) {
        if (m_immediate && (m_priority == ActivityForegroundPriority)) {
            err = rep.putBool(_T("foreground"), true);
            MojErrCheck(err);
        } else if (!m_immediate && (m_priority == ActivityBackgroundPriority)) {
            err = rep.putBool(_T("background"), true);
            MojErrCheck(err);
        } else {
            LOG_AM_DEBUG(
                    _T("[Activity %llu] Simple Activity type specified,"
                            " but immediate and priority were individually set, and will be individually output"),
                    m_id);
            useSimpleType = false;
        }
    }

    if (!useSimpleType) {
        err = rep.putBool(_T("immediate"), m_immediate);
        MojErrCheck(err);

        err = rep.putString(_T("priority"), ActivityPriorityNames[m_priority]);
        MojErrCheck(err);
    }

    if (m_continuous) {
        err = rep.putBool(_T("continuous"), true);
        MojErrCheck(err);
    }

    if (m_userInitiated) {
        err = rep.putBool(_T("userInitiated"), true);
        MojErrCheck(err);
    }

    if (m_powerActivity) {
        err = rep.putBool(_T("power"), true);
        MojErrCheck(err);
    }

    if (m_powerDebounce) {
        err = rep.putBool(_T("powerDebounce"), true);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr Activity::triggerToJson(MojObject& rep, unsigned flags) const
{
    MojErr err = MojErrNone;
    if (m_triggerVector.empty()) {
        return MojErrNotFound;
    }

    if (m_triggerMode == TriggerConditionType::kMultiTriggerNone) {
        return m_triggerVector.back()->toJson(rep, flags);
    }

    MojObject triggerArray(MojObject::TypeArray);
    for (auto& trigger : m_triggerVector) {
        MojObject triggerObj(MojObject::TypeObject);

        err = trigger->toJson(triggerObj, flags);
        MojErrCheck(err);

        err = triggerArray.push(triggerObj);
        MojErrCheck(err);
    }
    err = rep.put(_T((m_triggerMode == TriggerConditionType::kMultiTriggerAndMode ? "and" : "or")),
                  triggerArray);
    MojErrCheck(err);

    return err;
}

MojErr Activity::requirementsToJson(MojObject& rep, unsigned flags) const
{
    std::for_each(m_requirements.begin(),
                  m_requirements.end(),
                  [&] (const std::pair<std::string, std::shared_ptr<IRequirement>>& pair) {
                          pair.second->toJson(rep, flags);
                  });

    return MojErrNone;
}

MojErr Activity::internalToJson(MojObject& rep, unsigned flags) const
{
    MojErr err;

    err = rep.putInt(_T("m_id"), (MojInt64) m_id);
    MojErrCheck(err);

    err = rep.putBool(_T("m_persistent"), m_persistent);
    MojErrCheck(err);

    err = rep.putBool(_T("m_explicit"), m_explicit);
    MojErrCheck(err);

    err = rep.putBool(_T("m_immediate"), m_immediate);
    MojErrCheck(err);

    err = rep.putInt(_T("m_priority"), (MojInt64) m_priority);
    MojErrCheck(err);

    err = rep.putBool(_T("m_useSimpleType"), m_useSimpleType);
    MojErrCheck(err);

    err = rep.putBool(_T("m_continuous"), m_continuous);
    MojErrCheck(err);

    err = rep.putBool(_T("m_userInitiated"), m_userInitiated);
    MojErrCheck(err);

    err = rep.putBool(_T("m_powerDebounce"), m_powerDebounce);
    MojErrCheck(err);

    err = rep.putBool(_T("m_scheduled"), m_scheduled);
    MojErrCheck(err);

    err = rep.putBool(_T("m_ready"), m_ready);
    MojErrCheck(err);

    err = rep.putBool(_T("m_running"), m_running);
    MojErrCheck(err);

    err = rep.putBool(_T("m_ending"), m_ending);
    MojErrCheck(err);

    err = rep.putBool(_T("m_terminate"), m_terminate);
    MojErrCheck(err);

    err = rep.putBool(_T("m_restart"), m_restart);
    MojErrCheck(err);

    err = rep.putBool(_T("m_requeue"), m_requeue);
    MojErrCheck(err);

    err = rep.putBool(_T("m_yielding"), m_yielding);
    MojErrCheck(err);

    err = rep.putBool(_T("m_released"), m_released);
    MojErrCheck(err);

    return MojErrNone;
}

void Activity::pushIdentityJson(MojObject& array) const
{
    MojErr errs = MojErrNone;

    MojObject identity(MojObject::TypeObject);

    MojErr err = identityToJson(identity);
    MojErrAccumulate(errs, err);

    err = array.push(identity);
    MojErrAccumulate(errs, err);

    if (errs) {
        throw std::runtime_error("Unable to convert from Activity to JSON object");
    }
}
