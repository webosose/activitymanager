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

#ifndef __ABSTRACT_ACTIVITY_STATE_H__
#define __ABSTRACT_ACTIVITY_STATE_H__

#include <list>
#include <pbnjson.hpp>

#include "activity/schedule/Schedule.h"
#include "base/AbstractCallback.h"
#include "base/ITrigger.h"
#include "base/IRequirement.h"

class Activity;

class AbstractActivityState {
public:
    enum ActivityTransition {
        kActivityTransitionNone,
        kActivityTransitionCreating,
        kActivityTransitionStarting,
        kActivityTransitionResuming,
        kActivityTransitionPausing,
        kActivityTransitionSatisfying,
        kActivityTransitionExpiring,
        kActivityTransitionRestarting,
        kActivityTransitionFailing,
        kActivityTransitionDestroying,
    };

    AbstractActivityState();
    virtual ~AbstractActivityState() {};

    virtual void enter(std::shared_ptr<Activity> activity);

    virtual void create(std::shared_ptr<Activity> activity);
    virtual void start(std::shared_ptr<Activity> activity);
    virtual void pause(std::shared_ptr<Activity> activity);
    virtual void complete(std::shared_ptr<Activity> activity);
    virtual void destroy(std::shared_ptr<Activity> activity);

    virtual void onTriggerFail(
            std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger);
    virtual void onTriggerUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger);
    virtual void onCallbackFail(
            std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback);
    virtual void onCallbackSucceed(
            std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback);
    virtual void onRequirementUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement);
    virtual void onSchedule(
            std::shared_ptr<Activity> activity, std::shared_ptr<Schedule> schedule);

    virtual std::string getName() = 0;
    bool getActivityTransitionName(std::string& name);
    std::string toString();

protected:
    void setTransition(ActivityTransition transition, std::shared_ptr<Activity> activity);

    ActivityTransition m_transition;
};



///////////////////////////////////////////////////////////////////////////////////////////////////

class ActivityStateCreated : public AbstractActivityState {
public:
    static std::shared_ptr<ActivityStateCreated> newInstance()
    {
        return std::shared_ptr<ActivityStateCreated>(new ActivityStateCreated);
    }

    virtual ~ActivityStateCreated() {};

    virtual void start(std::shared_ptr<Activity> activity);
    virtual void pause(std::shared_ptr<Activity> activity);

    virtual std::string getName();

private:
    ActivityStateCreated() {};
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class ActivityStatePaused : public AbstractActivityState {
public:
    static std::shared_ptr<ActivityStatePaused> newInstance()
    {
        return std::shared_ptr<ActivityStatePaused>(new ActivityStatePaused);
    }

    virtual ~ActivityStatePaused() {};

    virtual void start(std::shared_ptr<Activity> activity);

    virtual std::string getName();

private:
    ActivityStatePaused() {};
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class ActivityStateUnsatisfied : public AbstractActivityState {
public:
    static std::shared_ptr<ActivityStateUnsatisfied> newInstance()
    {
        return std::shared_ptr<ActivityStateUnsatisfied>(new ActivityStateUnsatisfied);
    }

    virtual ~ActivityStateUnsatisfied() {};

    virtual void enter(std::shared_ptr<Activity> activity);

    virtual void pause(std::shared_ptr<Activity> activity);

    virtual void onTriggerUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger);
    virtual void onRequirementUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement);
    virtual void onSchedule(
            std::shared_ptr<Activity> activity, std::shared_ptr<Schedule> schedule);

    virtual std::string getName();

private:
    ActivityStateUnsatisfied() {};
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class ActivityStateSatisfied : public AbstractActivityState {
public:
    static std::shared_ptr<ActivityStateSatisfied> newInstance()
    {
        return std::shared_ptr<ActivityStateSatisfied>(new ActivityStateSatisfied);
    }

    virtual ~ActivityStateSatisfied() {};

    virtual void enter(std::shared_ptr<Activity> activity);

    void onRequirementUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement);

    virtual std::string getName();

private:
    ActivityStateSatisfied() {};
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class ActivityStateExpired : public AbstractActivityState {
public:
    static std::shared_ptr<ActivityStateExpired> newInstance()
    {
        return std::shared_ptr<ActivityStateExpired>(new ActivityStateExpired);
    }

    virtual ~ActivityStateExpired() {};

    virtual void complete(std::shared_ptr<Activity> activity);

    virtual void onCallbackSucceed(
            std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback);
    virtual void onRequirementUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement);

    virtual std::string getName();

private:
    ActivityStateExpired() {};
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class ActivityStateFailed : public AbstractActivityState {
public:
    static std::shared_ptr<ActivityStateFailed> newInstance()
    {
        return std::shared_ptr<ActivityStateFailed>(new ActivityStateFailed);
    }

    virtual ~ActivityStateFailed() {};

    virtual void enter(std::shared_ptr<Activity> activity);

    virtual void destroy(std::shared_ptr<Activity> activity);

    virtual void onTriggerFail(
            std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger);
    virtual void onTriggerUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<ITrigger> trigger);
    virtual void onCallbackFail(
            std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback);
    virtual void onRequirementUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement);
    virtual void onSchedule(
            std::shared_ptr<Activity> activity, std::shared_ptr<Schedule> schedule);

    virtual std::string getName();

private:
    ActivityStateFailed() {};

    static std::list<std::weak_ptr<Activity>> s_list;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class ActivityStateDestroyed : public AbstractActivityState {
public:
    static std::shared_ptr<ActivityStateDestroyed> newInstance()
    {
        return std::shared_ptr<ActivityStateDestroyed>(new ActivityStateDestroyed);
    }

    virtual ~ActivityStateDestroyed() {};

    virtual void enter(std::shared_ptr<Activity> activity);

    virtual void onCallbackFail(
            std::shared_ptr<Activity> activity, std::shared_ptr<AbstractCallback> callback);
    virtual void onRequirementUpdate(
            std::shared_ptr<Activity> activity, std::shared_ptr<IRequirement> requirement);

    virtual std::string getName();

private:
    ActivityStateDestroyed() {};
};

#endif /* __ABSTRACT_ACTIVITY_STATE_H__ */
