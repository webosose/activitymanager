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

#include "ActivityManager.h"

#include <boost/iterator/transform_iterator.hpp>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

#include "activity/ContinuousActivity.h"
#include "tools/ActivityMonitor.h"
#include "util/Logging.h"

const char *ActivityManager::kRunQueueNames[] = {
    "initialized",
    "scheduled",
    "ready",
    "readyInteractive",
    "background",
    "backgroundInteractive",
    "immediate",
    "ended"
};

const char* const ActivityManager::kClientName = "com.webos.service.activitymanager.client";
const char* const ActivityManager::kServiceName = "com.webos.service.activitymanager";

ActivityManager::ActivityManager()
    : m_enabled(kExternalEnable)
    , m_backgroundConcurrencyLevel(kDefaultBackgroundConcurrencyLevel)
    , m_backgroundInteractiveConcurrencyLevel(kDefaultBackgroundInteractiveConcurrencyLevel)
    , m_yieldTimeoutSeconds(kDefaultBackgroundInteractiveYieldSeconds)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    /* Activity ID 0 is reserved */
    m_nextActivityId = 1;
}

ActivityManager::~ActivityManager()
{
}

void ActivityManager::registerActivityId(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Registering ID", act->getId());

    ActivityMap::const_iterator found = m_activities.find(act->getId());
    if (found != m_activities.end()) {
        throw std::runtime_error("Activity ID is already registered");
    }

    m_activities[act->getId()] = act;
}

void ActivityManager::registerActivityName(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Registering as %s/\"%s\"", act->getId(),
                 act->getCreator().getString().c_str(), act->getName().c_str());

    bool success = m_nameTable.insert(*act).second;
    if (!success) {
        throw std::runtime_error("Activity name is already registered");
    }

    act->setState(ActivityStateCreated::newInstance());
}

void ActivityManager::unregisterActivityName(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Unregistering from %s/\"%s\"", act->getId(),
                 act->getCreator().getString().c_str(), act->getName().c_str());

    if (act->m_nameTableItem.is_linked()) {
        act->m_nameTableItem.unlink();
    } else {
        throw std::runtime_error("Activity name is not registered");
    }
}

std::shared_ptr<Activity> ActivityManager::getActivity(const std::string& name,
                                                       const BusId& creator)
{
    ActivityNameTable::iterator iter;

    if (creator.getType() == BusAnon) {
        iter = m_nameTable.find(ActivityKey(name, creator),
                                ActivityNameOnlyComp());
    } else {
        iter = m_nameTable.find(ActivityKey(name, creator), ActivityNameComp());
    }

    if (iter == m_nameTable.end()) {
        throw std::runtime_error("Activity name/creator pair not found");
    }

    return iter->shared_from_this();
}

std::shared_ptr<Activity> ActivityManager::getActivity(activityId_t id)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    ActivityMap::iterator iter = m_activities.find(id);
    if (iter != m_activities.end()) {
        return iter->second;
    }

    throw std::runtime_error("activityId not found");
}

std::shared_ptr<Activity> ActivityManager::getNewActivity(bool continuous)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    std::shared_ptr<Activity> act;

    /* Use sequential ID */
    while (true) {
        ActivityIdTable::const_iterator hint = m_idTable.lower_bound(m_nextActivityId, ActivityIdComp());
        if ((hint == m_idTable.end()) || (hint->getId() != m_nextActivityId)) {
            if (continuous) {
                act = std::make_shared<ContinuousActivity>(m_nextActivityId++);
            } else {
                act = std::make_shared<Activity>(m_nextActivityId++);
            }
            m_idTable.insert(hint, *act);
            break;
        } else {
            m_nextActivityId++;
        }
    }

    LOG_AM_DEBUG("[Activity %llu] Allocated", act->getId());

    return act;
}

std::shared_ptr<Activity> ActivityManager::getNewActivity(activityId_t id, bool continuous)
{
    LOG_AM_DEBUG("[Activity %llu] Forcing allocation", id);

    ActivityMap::iterator iter = m_activities.find(id);
    if (iter != m_activities.end()) {
        LOG_AM_WARNING(MSGID_SAME_ACTIVITY_ID_FOUND, 1,
                       PMLOGKFV("Activity","%llu",id), "");
    }

    std::shared_ptr<Activity> act;
    if (continuous) {
        act = std::make_shared<ContinuousActivity>(id);
    } else {
        act = std::make_shared<Activity>(id);
    }
    m_idTable.insert(*act);

    return act;
}

void ActivityManager::releaseActivity(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Releasing", act->getId());

    evictQueue(act);

    ActivityMap::iterator iter = m_activities.find(act->getId());
    if (iter == m_activities.end()) {
        LOG_AM_WARNING(
                MSGID_RELEASE_ACTIVITY_NOTFOUND, 1,
                PMLOGKFV("Activity", "%llu", act->getId()),
                "Not found in Activity table while attempting to release");
    } else if (iter->second == act) {
        m_activities.erase(iter);
    }

    checkReadyQueue();
}

ActivityManager::ActivityVec ActivityManager::getActivities() const
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    ActivityVec out;
    out.reserve(m_activities.size());

    std::transform(m_activities.begin(),
                   m_activities.end(),
                   std::back_inserter(out),
                   boost::bind(&ActivityMap::value_type::second, _1));

    return out;
}

MojErr ActivityManager::startActivity(std::shared_ptr<Activity> act, MojServiceMessage *msg)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Start", act->getId());

    act->setMessageForStart(msg);
    g_timeout_add_full(G_PRIORITY_HIGH, 0, &ActivityManager::_startActivity, act.get(), NULL);

    return MojErrNone;
}

gboolean ActivityManager::_startActivity(gpointer data)
{
    Activity* activity = static_cast<Activity*>(data);
    if (!activity) {
        LOG_AM_ERROR(MSGID_ACTIVITYID_NOT_FOUND, 0, "%s", "activity is null");
        return G_SOURCE_REMOVE;
    }
    std::shared_ptr<Activity> self;
    try {
        self = ActivityManager::getInstance().getActivity(activity->getId());
    } catch (const std::runtime_error& except) {
        LOG_AM_ERROR(MSGID_ACTIVITYID_NOT_FOUND, 0, "%s", except.what());
        return G_SOURCE_REMOVE;
    }

    try {
        self->getState()->start(self);
    } catch (const std::runtime_error& except) {
        self->respondForStart(std::string("Failed to start activity: ") + except.what());
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_REMOVE;
}

MojErr ActivityManager::stopActivity(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Stop", act->getId());

    act->getState()->destroy(act);

    return MojErrNone;
}

MojErr ActivityManager::cancelActivity(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Cancel", act->getId());

    act->getState()->destroy(act);

    return MojErrNone;
}

MojErr ActivityManager::pauseActivity(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Pause", act->getId());

    act->getState()->pause(act);

    return MojErrNone;
}

void ActivityManager::enable(unsigned mask)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (mask & kExternalEnable) {
        LOG_AM_DEBUG("Enabling Activity Manager: External");
    }

    if (mask & kUIEnable) {
        LOG_AM_DEBUG("Enabling Activity Manager: Device UI enabled");
    }

    if (mask & kServiceOnline) {
        LOG_AM_DEBUG("Enabling Activity Manager: Service online");
    }

    if ((mask & kEnableMask) != mask) {
        LOG_AM_DEBUG("Unknown bits set in mask in call to Enable: %x", mask);
    }

    m_enabled |= (mask & kEnableMask);

    if (isEnabled()) {
        scheduleAllActivities();
    }
}

void ActivityManager::disable(unsigned mask)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (mask & kExternalEnable) {
        LOG_AM_DEBUG("Disabling Activity Manager: External");
    }

    if (mask & kUIEnable) {
        LOG_AM_DEBUG("Disabling Activity Manager: Device UI disabled");
    }

    if (mask & kServiceOnline) {
        LOG_AM_DEBUG("Disabling Activity Manager: Service online");
    }

    if ((mask & kEnableMask) != mask) {
        LOG_AM_DEBUG("Unknown bits set in mask in call to Disable: %x", mask);
    }

    m_enabled &= ~mask;
}

bool ActivityManager::isEnabled() const
{
    /* All bits must be enabled */
    return (m_enabled & kEnableMask) == kEnableMask;
}

void ActivityManager::informActivityInitialized(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Initialized and ready to be scheduled", act->getId());

    /* If an Activity is restarting, it will be parked (temporarily) in
     * the ended queue. */
    if (act->m_runQueueItem.is_linked()) {
        act->m_runQueueItem.unlink();
    }

    /* If the Activity Manager isn't enabled yet, just queue the Activities.
     * otherwise, schedule them immediately. */
    if (isEnabled()) {
        m_runQueue[RunQueueScheduled].push_back(*act);
        act->scheduleActivity();
    } else {
        m_runQueue[RunQueueInitialized].push_back(*act);
    }
}

void ActivityManager::informActivityReady(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Now ready to run", act->getId());

    if (act->m_runQueueItem.is_linked()) {
        act->m_runQueueItem.unlink();
    } else {
        LOG_AM_DEBUG("[Activity %llu] not found on any run queue when moving to ready state", act->getId());
    }

    if (act->isImmediate()) {
        m_runQueue[RunQueueImmediate].push_back(*act);
        runActivity(*act);
        return;
    }

    if (act->isUserInitiated()) {
        m_runQueue[RunQueueReadyInteractive].push_back(*act);
    } else {
        m_runQueue[RunQueueReady].push_back(*act);
    }

    checkReadyQueue();
}

void ActivityManager::informActivityNotReady(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] No longer ready to run", act->getId());

    if (act->m_runQueueItem.is_linked()) {
        act->m_runQueueItem.unlink();
    } else {
        LOG_AM_DEBUG("[Activity %llu] not found on any run queue when moving to not ready state", act->getId());
    }

    m_runQueue[RunQueueScheduled].push_back(*act);
}

void ActivityManager::informActivityRunning(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Running", act->getId());
}

void ActivityManager::informActivityEnding(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Ending", act->getId());

    /* Nothing to do here yet, it still has subscribers who may have processing
     * to do. */
}

void ActivityManager::informActivityEnd(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Activity %llu] Has ended", act->getId());

    /* If Activity was never fully initialized, it's ok for it not to be on
     * a queue here */
    if (act->m_runQueueItem.is_linked()) {
        act->m_runQueueItem.unlink();
    }

    m_runQueue[RunQueueEnded].push_back(*act);

    /* Could be room to run more background Activities */
    checkReadyQueue();
}

void ActivityManager::scheduleAllActivities()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Scheduling all Activities");

    while (!m_runQueue[RunQueueInitialized].empty()) {
        Activity& act = m_runQueue[RunQueueInitialized].front();
        act.m_runQueueItem.unlink();

        LOG_AM_DEBUG("Granting [Activity %llu] permission to schedule", act.getId());

        m_runQueue[RunQueueScheduled].push_back(act);
        act.scheduleActivity();
    }
}

void ActivityManager::evictQueue(std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (act->m_runQueueItem.is_linked()) {
        LOG_AM_DEBUG("[Activity %llu] evicted from run queue on release", act->getId());
        act->m_runQueueItem.unlink();
    }
}

void ActivityManager::runActivity(Activity& act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Running [Activity %llu]", act.getId());

    act.runActivity();
}

void ActivityManager::runReadyBackgroundActivity(Activity& act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Running background [Activity %llu]", act.getId());

    if (act.m_runQueueItem.is_linked()) {
        act.m_runQueueItem.unlink();
    } else {
        LOG_AM_WARNING(MSGID_ATTEMPT_RUN_BACKGRND_ACTIVITY, 1,
                       PMLOGKFV("Activity","%llu",act.getId()), "");
    }

    m_runQueue[RunQueueBackground].push_back(act);

    runActivity(act);
}

void ActivityManager::runReadyBackgroundInteractiveActivity(Activity& act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Running background interactive [Activity %llu]", act.getId());

    if (act.m_runQueueItem.is_linked()) {
        act.m_runQueueItem.unlink();
    } else {
        LOG_AM_DEBUG(
                "[Activity %llu] was not queued attempting to run background interactive Activity",
                act.getId());
    }

    m_runQueue[RunQueueBackgroundInteractive].push_back(act);

    runActivity(act);
}

unsigned ActivityManager::getRunningBackgroundActivitiesCount() const
{
    return (unsigned) (m_runQueue[RunQueueBackground].size() +
            m_runQueue[RunQueueBackgroundInteractive].size());
}

void ActivityManager::checkReadyQueue()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Checking to see if more background Activities can run");

    bool ranInteractive = false;

    while (((getRunningBackgroundActivitiesCount() < m_backgroundInteractiveConcurrencyLevel) ||
            (m_backgroundInteractiveConcurrencyLevel == kUnlimitedBackgroundConcurrency)) &&
            !m_runQueue[RunQueueReadyInteractive].empty()) {
        runReadyBackgroundInteractiveActivity(m_runQueue[RunQueueReadyInteractive].front());
        ranInteractive = true;
    }

    if (!m_runQueue[RunQueueReadyInteractive].empty()) {
        if (ranInteractive) {
            updateYieldTimeout();
        } else if (!m_interactiveYieldTimeout) {
            updateYieldTimeout();
        }
    } else if (m_interactiveYieldTimeout){
        cancelYieldTimeout();
    }

    while (((getRunningBackgroundActivitiesCount() < m_backgroundConcurrencyLevel) ||
            (m_backgroundConcurrencyLevel == kUnlimitedBackgroundConcurrency)) &&
            !m_runQueue[RunQueueReady].empty()) {
        runReadyBackgroundActivity(m_runQueue[RunQueueReady].front());
    }
}

void ActivityManager::updateYieldTimeout()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (!m_interactiveYieldTimeout) {
        LOG_AM_DEBUG(
                "Arming background interactive yield timeout for %u seconds",
                m_yieldTimeoutSeconds);
    } else {
        LOG_AM_DEBUG(
                "Updating background interactive yield timeout for %u seconds",
                m_yieldTimeoutSeconds);
    }

    m_interactiveYieldTimeout = std::make_shared<TimeoutPtr<ActivityManager>>(
            this,
            m_yieldTimeoutSeconds,
            &ActivityManager::interactiveYieldTimeout);
    m_interactiveYieldTimeout->arm();
}

void ActivityManager::cancelYieldTimeout()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Cancelling background interactive yield timeout");

    m_interactiveYieldTimeout.reset();
}

void ActivityManager::interactiveYieldTimeout()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Background interactive yield timeout triggered");

    if (m_runQueue[RunQueueReadyInteractive].empty()) {
        LOG_AM_DEBUG("Ready interactive queue is empty, cancelling yield timeout");
        cancelYieldTimeout();
        return;
    }

    /* XXX make 1 more Activity yield, but only if there are fewer Activities
     * yielding than waiting in the interactive queue. */
    unsigned waiting = (unsigned) m_runQueue[RunQueueReadyInteractive].size();
    unsigned yielding = 0;

    std::shared_ptr<Activity> victim;

    ActivityRunQueue::iterator iter;

    for (iter = m_runQueue[RunQueueBackgroundInteractive].begin();
            iter != m_runQueue[RunQueueBackgroundInteractive].end() ; ++iter) {
        if (iter->isYielding()) {
            if (++yielding >= waiting) {
                break;
            }
        } else {
            if (!victim) {
                victim = iter->shared_from_this();
            }
        }
    }

    /* If there aren't already too many yielding... */
    if (iter == m_runQueue[RunQueueBackgroundInteractive].end()) {
        if (victim) {
            LOG_AM_DEBUG("Requesting that [Activity %llu] yield", victim->getId());
            victim->yieldActivity();
        } else {
            LOG_AM_DEBUG("All running background interactive Activities are already yielding");
        }
    } else {
        LOG_AM_DEBUG(
                "Number of yielding Activities is already equal to the number of ready interactive Activities waiting in the queue");
    }

    updateYieldTimeout();
}

bool ActivityManager::ActivityNameComp::operator()(const Activity& act1,
                                                   const Activity& act2) const
{
    const std::string& act1Name = act1.getName();
    const std::string& act2Name = act2.getName();

    if (act1Name != act2Name) {
        return act1Name < act2Name;
    } else {
        return act1.getCreator() < act2.getCreator();
    }
}

bool ActivityManager::ActivityNameComp::operator()(const ActivityKey& key,
                                                   const Activity& act) const
{
    return key < ActivityKey(act.getName(), act.getCreator());
}

bool ActivityManager::ActivityNameComp::operator()(const Activity& act,
                                                   const ActivityKey& key) const
{
    return ActivityKey(act.getName(), act.getCreator()) < key;
}

bool ActivityManager::ActivityNameOnlyComp::operator()(
        const ActivityKey& key, const Activity& act) const
{
    return key.first < act.getName();
}

bool ActivityManager::ActivityNameOnlyComp::operator()(
        const Activity& act, const ActivityKey& key) const
{
    return act.getName() < key.first;
}

bool ActivityManager::ActivityIdComp::operator()(const Activity& act1,
                                                 const Activity& act2) const
{
    return act1.getId() < act2.getId();
}

bool ActivityManager::ActivityIdComp::operator()(const activityId_t& id,
                                                 const Activity& act) const
{
    return id < act.getId();
}

bool ActivityManager::ActivityIdComp::operator()(const Activity& act,
                                                 const activityId_t& id) const
{
    return act.getId() < id;
}

MojErr ActivityManager::infoToJson(MojObject& rep) const
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    MojErr err = MojErrNone;

    /* Scan the various run queues of the Activity Manager */
    MojObject queues(MojObject::TypeArray);

    for (int i = 0 ; i < RunQueueMax ; i++) {
        if (!m_runQueue[i].empty()) {
            MojObject activities(MojObject::TypeArray);

            std::for_each(
                    m_runQueue[i].begin(),
                    m_runQueue[i].end(),
                    boost::bind(&Activity::pushIdentityJson, _1, std::ref(activities)));

            MojObject queue;
            err = queue.putString(_T("name"), kRunQueueNames[i]);
            MojErrCheck(err);

            err = queue.put(_T("activities"), activities);
            MojErrCheck(err);

            err = queues.push(queue);
            MojErrCheck(err);
        }
    }

    if (!queues.empty()) {
        err = rep.put(_T("queues"), queues);
        MojErrCheck(err);
    }

    std::vector<std::shared_ptr<const Activity> > leaked;

    std::shared_ptr<const Activity> (Activity::*shared_from_this_ptr)() const = &Activity::shared_from_this;
    std::set_difference(
            boost::make_transform_iterator(m_idTable.cbegin(), boost::bind(shared_from_this_ptr, _1)),
            boost::make_transform_iterator(m_idTable.cend(), boost::bind(shared_from_this_ptr, _1)),
            boost::make_transform_iterator(m_activities.begin(), boost::bind(&ActivityMap::value_type::second, _1)),
            boost::make_transform_iterator(m_activities.end(), boost::bind(&ActivityMap::value_type::second, _1)),
            std::back_inserter(leaked));

    if (!leaked.empty()) {
        MojObject leakedActivities(MojObject::TypeArray);

        std::for_each(
                leaked.begin(),
                leaked.end(),
                boost::bind(&Activity::pushIdentityJson, _1, std::ref(leakedActivities)));

        err = rep.put(_T("leakedActivities"), leakedActivities);
        MojErrCheck(err);
    }

    return MojErrNone;
}

