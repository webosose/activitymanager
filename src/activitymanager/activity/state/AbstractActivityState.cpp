// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "activity/Activity.h"
#include "activity/ActivityManager.h"
#include "conf/ActivityTypes.h"
#include "conf/Config.h"
#include "tools/ActivityMonitor.h"
#include "util/Logging.h"

AbstractActivityState::AbstractActivityState()
    : m_transition(kActivityTransitionNone)
{
}

void AbstractActivityState::enter(std::shared_ptr<Activity> activity)
{
}

void AbstractActivityState::create(std::shared_ptr<Activity> activity)
{
    LOG_AM_WARNING("STATE_TRANSITION_FAIL", 3,
                   PMLOGKFV("Activity", "%llu", activity->getId()),
                   PMLOGKS("state", toString().c_str()),
                   PMLOGKS("func", __FUNCTION__), "");

    throw std::runtime_error("Invalid transition: " + toString() + " -> " + __FUNCTION__);
}

void AbstractActivityState::start(std::shared_ptr<Activity> activity)
{
    LOG_AM_WARNING("STATE_TRANSITION_FAIL", 3,
                   PMLOGKFV("Activity", "%llu", activity->getId()),
                   PMLOGKS("state", toString().c_str()),
                   PMLOGKS("func", __FUNCTION__), "");

    throw std::runtime_error("Invalid transition: " + toString() + " -> " + __FUNCTION__);
}

void AbstractActivityState::pause(std::shared_ptr<Activity> activity)
{
    LOG_AM_WARNING("STATE_TRANSITION_FAIL", 3,
                   PMLOGKFV("Activity", "%llu", activity->getId()),
                   PMLOGKS("state", toString().c_str()),
                   PMLOGKS("func", __FUNCTION__), "");

    throw std::runtime_error("Invalid transition: " + toString() + " -> " + __FUNCTION__);
}

void AbstractActivityState::complete(std::shared_ptr<Activity> activity)
{
    LOG_AM_WARNING("STATE_TRANSITION_FAIL", 3,
                   PMLOGKFV("Activity", "%llu", activity->getId()),
                   PMLOGKS("state", toString().c_str()),
                   PMLOGKS("func", __FUNCTION__), "");

    throw std::runtime_error("Invalid transition: " + toString() + " -> " + __FUNCTION__);
}

void AbstractActivityState::destroy(std::shared_ptr<Activity> activity)
{
    setTransition(kActivityTransitionDestroying, activity);
    activity->unscheduleActivity();
    activity->setState(ActivityStateDestroyed::newInstance());
}

void AbstractActivityState::onTriggerFail(
        std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger)
{
    ActivityMonitor::getInstance().send(activity);

    LOG_AM_WARNING("TRIGGER_FAIL", 4,
                   PMLOGKFV("Activity", "%llu", activity->getId()),
                   PMLOGKFV("continuous", "%d", activity->isContinuous()),
                   PMLOGKS("state", toString().c_str()),
                   PMLOGKS("trigger", trigger->getName().c_str()), "");

    setTransition(kActivityTransitionFailing, activity);
    activity->setState(ActivityStateFailed::newInstance());
}

void AbstractActivityState::onTriggerUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger)
{
    ActivityMonitor::getInstance().send(activity);
}

void AbstractActivityState::onCallbackFail(
        std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback)
{
    ActivityMonitor::getInstance().send(activity);

    LOG_AM_WARNING("CALLBACK_FAIL", 3,
                   PMLOGKFV("Activity", "%llu", activity->getId()),
                   PMLOGKFV("continuous", "%d", activity->isContinuous()),
                   PMLOGKS("state", toString().c_str()), "");

    setTransition(kActivityTransitionFailing, activity);
    activity->setState(ActivityStateFailed::newInstance());
}

void AbstractActivityState::onCallbackSucceed(
        std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback)
{
    // do nothing
}

void AbstractActivityState::onRequirementUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement)
{
    ActivityMonitor::getInstance().send(activity);
    activity->broadcastEvent(kActivityUpdateEvent); // backward compatiblity
}

void AbstractActivityState::onSchedule(
        std::shared_ptr<Activity> activity, std::shared_ptr<Schedule> schedule)
{
    ActivityMonitor::getInstance().send(activity);
}

void AbstractActivityState::setTransition(
        ActivityTransition transition, std::shared_ptr<Activity> activity)
{
    m_transition = transition;
    LOG_AM_DEBUG("[Activity %llu] Transition \"%s\"", activity->getId(), toString().c_str());

    ActivityMonitor::getInstance().send(activity);

    // backward compatibility
    switch (transition) {
        case kActivityTransitionPausing:
            activity->broadcastEvent(kActivityPauseEvent);
            break;
        case kActivityTransitionSatisfying:
            activity->broadcastEvent(kActivityStartEvent);
            break;
        case kActivityTransitionRestarting:
            activity->broadcastEvent(kActivityCompleteEvent);
            break;
        case kActivityTransitionDestroying:
            activity->broadcastEvent(kActivityCancelEvent);
            break;
        default:
            break;
    }
}

bool AbstractActivityState::getActivityTransitionName(std::string& name)
{
    switch (m_transition) {
        case kActivityTransitionCreating:
            name = "creating";
            return true;
        case kActivityTransitionStarting:
            name = "starting";
            return true;
        case kActivityTransitionResuming:
            name = "resuming";
            return true;
        case kActivityTransitionPausing:
            name = "pausing";
            return true;
        case kActivityTransitionSatisfying:
            name = "satisfying";
            return true;
        case kActivityTransitionExpiring:
            name = "expiring";
            return true;
        case kActivityTransitionRestarting:
            name = "restarting";
            return true;
        case kActivityTransitionFailing:
            name = "failing";
            return true;
        case kActivityTransitionDestroying:
            name = "destroying";
            return true;
        default:
            name = "";
            return false;
    }
}

std::string AbstractActivityState::toString()
{
    std::string transition;
    if (!getActivityTransitionName(transition)) {
        return getName();
    }
    return transition;
}

void ActivityStateCreated::start(std::shared_ptr<Activity> activity)
{
    setTransition(kActivityTransitionStarting, activity);
    activity->requestScheduleActivity();
    activity->setState(ActivityStateUnsatisfied::newInstance());
}

void ActivityStateCreated::pause(std::shared_ptr<Activity> activity)
{
    setTransition(kActivityTransitionPausing, activity);
    activity->setState(ActivityStatePaused::newInstance());
}

std::string ActivityStateCreated::getName()
{
    return "created";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityStatePaused::start(std::shared_ptr<Activity> activity)
{
    setTransition(kActivityTransitionResuming, activity);
    activity->requestScheduleActivity();
    activity->setState(ActivityStateUnsatisfied::newInstance());
}

std::string ActivityStatePaused::getName()
{
    return "paused";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityStateUnsatisfied::enter(std::shared_ptr<Activity> activity)
{
    if (activity->isRunnable()) {
        setTransition(kActivityTransitionSatisfying, activity);
        activity->setState(ActivityStateSatisfied::newInstance());
    }
}

void ActivityStateUnsatisfied::pause(std::shared_ptr<Activity> activity)
{
    setTransition(kActivityTransitionPausing, activity);
    activity->setState(ActivityStatePaused::newInstance());
}

void ActivityStateUnsatisfied::onTriggerUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger)
{
    AbstractActivityState::onTriggerUpdate(activity, trigger);

    if (activity->isRunnable()) {
        setTransition(kActivityTransitionSatisfying, activity);
        activity->setState(ActivityStateSatisfied::newInstance());
    }
}

void ActivityStateUnsatisfied::onRequirementUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement)
{
    AbstractActivityState::onRequirementUpdate(activity, requirement);

    if (activity->isRunnable()) {
        setTransition(kActivityTransitionSatisfying, activity);
        activity->setState(ActivityStateSatisfied::newInstance());
    }
}

void ActivityStateUnsatisfied::onSchedule(
        std::shared_ptr<Activity> activity, std::shared_ptr<Schedule> schedule)
{
    AbstractActivityState::onSchedule(activity, schedule);

    if (activity->isRunnable()) {
        setTransition(kActivityTransitionSatisfying, activity);
        activity->setState(ActivityStateSatisfied::newInstance());
    }
}

std::string ActivityStateUnsatisfied::getName()
{
    return "unsatisfied";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityStateSatisfied::enter(std::shared_ptr<Activity> activity)
{
    setTransition(kActivityTransitionExpiring, activity);
    activity->requestRunActivity();
    activity->unscheduleActivity();
    activity->setState(ActivityStateExpired::newInstance());
}

void ActivityStateSatisfied::onRequirementUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement)
{
    if (activity->isContinuous()) {
        ActivityMonitor::getInstance().send(activity);
    }
}

std::string ActivityStateSatisfied::getName()
{
    return "satisfied";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityStateExpired::complete(std::shared_ptr<Activity> activity)
{
    if (activity->m_terminate) {
        if (activity->isSubscribed()) {
            // backward compatibility
            activity->broadcastEvent(kActivityCompleteEvent);
        } else {
            setTransition(kActivityTransitionDestroying, activity);
            activity->unscheduleActivity();
            activity->setState(ActivityStateDestroyed::newInstance());
        }
    } else {
        if (activity->isFastRestart()) {
            setTransition(kActivityTransitionFailing, activity);
            activity->setState(ActivityStateFailed::newInstance());
        } else {
            setTransition(kActivityTransitionRestarting, activity);
            activity->requestScheduleActivity();
            activity->setState(ActivityStateUnsatisfied::newInstance());
        }
    }
}

void ActivityStateExpired::onCallbackSucceed(
        std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback)
{
    if (activity->isContinuous()) {
        if (activity->isFastRestart()) {
            setTransition(kActivityTransitionFailing, activity);
            activity->setState(ActivityStateFailed::newInstance());
        } else {
            setTransition(kActivityTransitionRestarting, activity);
            activity->setState(ActivityStateUnsatisfied::newInstance());
        }
    }
}

void ActivityStateExpired::onRequirementUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement)
{
    if (activity->isContinuous()) {
        ActivityMonitor::getInstance().send(activity);
    }
}

std::string ActivityStateExpired::getName()
{
    return "expired";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

std::list<std::weak_ptr<Activity>> ActivityStateFailed::s_list;

void ActivityStateFailed::enter(std::shared_ptr<Activity> activity)
{
    activity->unscheduleActivity();

    s_list.push_back(activity);
    if (s_list.size() > Config::getInstance().getFailedLimitCount()) {
        if (!s_list.front().expired()) {
            std::shared_ptr<Activity> oldest = s_list.front().lock();
            oldest->getState()->destroy(oldest);
        }
        s_list.pop_front();
    }
}

void ActivityStateFailed::destroy(std::shared_ptr<Activity> activity)
{
    setTransition(kActivityTransitionDestroying, activity);
    activity->unscheduleActivity();
    activity->setState(ActivityStateDestroyed::newInstance());
}

void ActivityStateFailed::onTriggerFail(
        std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger)
{
    // do nothing
}

void ActivityStateFailed::onTriggerUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger)
{
    // do nothing
}

void ActivityStateFailed::onCallbackFail(
        std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback)
{
    // do nothing
}

void ActivityStateFailed::onRequirementUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement)
{
    // do nothing
}

void ActivityStateFailed::onSchedule(
        std::shared_ptr<Activity> activity, std::shared_ptr<Schedule> schedule)
{
    // do nothing
}

std::string ActivityStateFailed::getName()
{
    return "failed";
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityStateDestroyed::enter(std::shared_ptr<Activity> activity)
{
    activity->m_ending = true; // TODO: need to check
    activity->unscheduleActivity();
    ActivityManager::getInstance().informActivityEnd(activity);
    ActivityManager::getInstance().releaseActivity(activity);
}

void ActivityStateDestroyed::onCallbackFail(
        std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback)
{
    // do nothing
}

void ActivityStateDestroyed::onRequirementUpdate(
        std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement)
{
    // do nothing
}

std::string ActivityStateDestroyed::getName()
{
    return "destroyed";
}
