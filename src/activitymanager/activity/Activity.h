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

#ifndef __ACTIVITY_H__
#define __ACTIVITY_H__

#include <set>
#include <map>
#include <list>
#include <string>
#include <vector>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>
#include <pbnjson.hpp>

#include "Main.h"
#include "state/AbstractActivityState.h"
#include "type/AbstractPowerActivity.h"
#include "activity/schedule/Schedule.h"
#include "base/AbstractCallback.h"
#include "base/AbstractSubscription.h"
#include "base/BusId.h"
#include "base/IRequirement.h"
#include "base/ITrigger.h"
#include "base/Subscriber.h"
#include "conf/ActivityTypes.h"
#include "db/AbstractPersistCommand.h"
#include "db/PersistToken.h"

class ActivityManager;

enum TriggerConditionType {
    kMultiTriggerNone = 0,
    kMultiTriggerOrMode,
    kMultiTriggerAndMode
};

class Activity: public std::enable_shared_from_this<Activity>, public IRequirementListener {
public:
    Activity(activityId_t id);
    virtual ~Activity();

    /* Accessors for external properties */
    activityId_t getId() const;

    void setName(const std::string& name);
    const std::string& getName() const;
    bool isNameRegistered() const;

    void setDescription(const std::string& description);
    const std::string& getDescription() const;

    void setCreator(const Subscriber& creator);
    void setCreator(const BusId& creator);
    const BusId& getCreator() const;

    void setImmediate(bool immediate);
    bool isImmediate() const;

    /* Subscription/Subscriber management */
    MojErr addSubscription(std::shared_ptr<AbstractSubscription> sub);
    MojErr removeSubscription(std::shared_ptr<AbstractSubscription> sub);
    bool isSubscribed() const;

    /* Subscription messaging & control */
    MojErr broadcastEvent(ActivityEvent event);
    void plugAllSubscriptions();
    void unplugAllSubscriptions();

    /* Trigger Management */
    void setTriggerVector(std::vector<std::shared_ptr<ITrigger>> vec);
    std::vector<std::shared_ptr<ITrigger>> getTriggers() const;
    void clearTrigger();
    bool hasTrigger() const;
    void onFailTrigger(std::shared_ptr<ITrigger> trigger);
    virtual void onSuccessTrigger(std::shared_ptr<ITrigger> trigger, bool statusChanged, bool valueChanged);

    bool isTriggered() const;
    TriggerConditionType getTriggerMode();
    void setTriggerMode(TriggerConditionType mode);
    std::shared_ptr<ITrigger> getTrigger(std::string name);

    /* Callback Management */
    void setCallback(std::shared_ptr<AbstractCallback> callback);
    std::shared_ptr<AbstractCallback> getCallback();
    bool hasCallback() const;
    void callbackFailed(std::shared_ptr<AbstractCallback> callback,
                        AbstractCallback::FailureType failure);
    virtual void callbackSucceeded(std::shared_ptr<AbstractCallback> callback);

    /* Schedule Management */
    void setSchedule(std::shared_ptr<Schedule> schedule);
    void clearSchedule();
    virtual void scheduled();
    bool isScheduled() const;

    /* Requirement Management */
    void addRequirement(std::shared_ptr<IRequirement> requirement);
    void removeRequirement(const std::string& name);
    bool isRequirementSet(const std::string& name) const;
    bool hasRequirements() const;
    bool isRequirementMet() const;
    std::shared_ptr<IRequirement> getRequirement(std::string name);

    /* Requirement Listener */
    virtual void onMetRequirement(std::string requirement);
    void onUnmetRequirement(std::string requirement);
    void onUpdateRequirement(std::string requirement);

    /* Parent Management */
    void setParent(std::shared_ptr<AbstractSubscription> sub);
    BusId getParent() const;
    bool hasParent() const;
    MojErr adopt(std::shared_ptr<AbstractSubscription> sub, bool wait, bool *adopted = NULL);
    MojErr release(const BusId& caller);
    bool isReleased() const;

    MojErr complete(const BusId& caller, bool force = false);

    /* Persistence Management */
    void setPersistent(bool persistent);
    bool isPersistent() const;

    void setPersistToken(std::shared_ptr<PersistToken> token);
    std::shared_ptr<PersistToken> getPersistToken();
    void clearPersistToken();
    bool isPersistTokenSet() const;

    void hookPersistCommand(std::shared_ptr<AbstractPersistCommand> cmd);
    void unhookPersistCommand(std::shared_ptr<AbstractPersistCommand> cmd);
    bool isPersistCommandHooked() const;
    std::shared_ptr<AbstractPersistCommand> getHookedPersistCommand();

    /* Priority management */
    void setPriority(ActivityPriority_t priority);
    ActivityPriority_t getPriority() const;

    /* Explicit status Management */
    void setExplicit(bool explicitActivity);
    bool isExplicit() const;

    void setTerminateFlag(bool terminate);
    void setRestartFlag(bool restart);

    /* Use simple 'foreground' or 'background' */
    void setUseSimpleType(bool useSimpleType);

    /* Additional hints: Continuous and User Initiated control */
    void setContinuous(bool continuous);
    bool isContinuous() const;

    void setUserInitiated(bool userInitiated);
    bool isUserInitiated() const;

    /* Power management control */
    void setPowerActivity(std::shared_ptr<AbstractPowerActivity> powerActivity);
    std::shared_ptr<AbstractPowerActivity> getPowerActivity();
    bool isPowerActivity() const;
    void powerLockedNotification();
    void powerUnlockedNotification();

    /* Power debounce control */
    void setPowerDebounce(bool powerDebounce);
    bool isPowerDebounce() const;

    /* Activity external metadata manipulation */
    void setMetadata(const std::string& metadata);
    void clearMetadata();

    pbnjson::JValue requirementsToJson() const;
    pbnjson::JValue triggerToJson() const;
    pbnjson::JValue toJson() const;

    MojErr identityToJson(MojObject& rep) const;
    MojErr activityInfoToJson(MojObject& rep) const;
    MojErr toJson(MojObject& rep, unsigned flags) const;
    MojErr typeToJson(MojObject& rep, unsigned flags) const;
    MojErr triggerToJson(MojObject& rep, unsigned flags) const;
    MojErr requirementsToJson(MojObject& rep, unsigned flags) const;
    MojErr internalToJson(MojObject& rep, unsigned flags) const;

    void pushIdentityJson(MojObject& array) const;

    bool isRunning() const;
    bool isYielding() const;

    std::shared_ptr<AbstractActivityState> getState(void) const;
    void setState(std::shared_ptr<AbstractActivityState> state);

    void setMessageForStart(MojServiceMessage *msg);
    void respondForStart(std::string errorText = "");

protected:
    /* Activity Manager may access "private" control interfaces. */
    friend class ActivityManager;
    friend class AbstractActivityState;
    friend class ActivityStateNone;
    friend class ActivityStateCreated;
    friend class ActivityStateUnsatisfied;
    friend class ActivityStatePaused;
    friend class ActivityStateSatisfied;
    friend class ActivityStateExpired;
    friend class ActivityStateFailed;
    friend class ActivityStateDestroyed;

    void restartActivity();
    void requestScheduleActivity();
    void scheduleActivity();
    virtual void unscheduleActivity();
    void requestRunActivity();
    void runActivity();
    void requestRequeueActivity();
    void requeueActivity();
    void yieldActivity();
    void endActivity();

    virtual bool isRunnable() const;
    bool shouldRestart() const;
    bool shouldRequeue() const;

    void orphaned();
    void abandoned();

    void doAdopt();
    void doRunActivity();
    virtual void doCallback();

    bool isFastRestart();

    static gboolean setStateInternal(gpointer data);

    /* DISALLOW */
    Activity();
    Activity(const Activity& copy);
    Activity& operator=(const Activity& copy);
    bool operator==(const Activity& rhs) const;

protected:
    typedef boost::intrusive::set_member_hook<
            boost::intrusive::link_mode<boost::intrusive::auto_unlink>> ActivityTableItem;

    typedef boost::intrusive::list_member_hook<
            boost::intrusive::link_mode<boost::intrusive::auto_unlink>> ActivityListItem;

    typedef boost::intrusive::member_hook<Subscriber, Subscriber::SetItem,
            &Subscriber::m_item> SubscriberSetOption;
    typedef boost::intrusive::multiset<Subscriber, SubscriberSetOption,
            boost::intrusive::constant_time_size<false>> SubscriberSet;

    typedef boost::intrusive::member_hook<AbstractSubscription,
            AbstractSubscription::SetItem, &AbstractSubscription::m_item> SubscriptionSetOption;
    typedef boost::intrusive::set<AbstractSubscription, SubscriptionSetOption,
            boost::intrusive::constant_time_size<false>> SubscriptionSet;

    typedef std::list<std::shared_ptr<AbstractPersistCommand>> CommandQueue;

    typedef std::list<std::weak_ptr<AbstractSubscription>> AdopterQueue;

    /* External properties */
    std::string m_name;
    std::string m_description;
    activityId_t m_id;
    BusId m_creator;

    /* Externally supplied metadata in JSON format.  This will be stored
     * and returned with the Activity, and allows for more flexible tagging
     * and decoration of the Activity. */
    std::string m_metadata;

    /* The Activity will be stored in the Open webOS database.  Any command
     * that causes an update to the Activity's persisted state will wait until
     * that update has been acknowledged by MojoDB before returning to the
     * caller (or otherwise generating externally visible events). */
    bool m_persistent;

    /* The Activity must be explicitly terminated by command, rather than
     * implicitly cancelled if its parent unsubscribes.  If the parent of
     * an explicit Activity unsubscribes, cancel events are generated to the
     * remaining subscribers.  After all subscribers unsubscribe, the Activity
     * will be automatically rescheduled to run again. */
    bool m_explicit;

    /* Activity should begin running (callback called, 'start' event generated)
     * as soon as its prerequisites are met, without delay.  Otherwise, the
     * Activity Manager may delay the Activity while other Activities run,
     * and may change the order that those Activities are executed in. */
    bool m_immediate;

    /* Priority of Activity: "highest", "high", "normal", "low", "lowest".
     * Generally not directly set - just "background" ("low"), or
     * "foreground" ("immediate", "normal"). */
    ActivityPriority_t m_priority;

    /* If 'immediate' and 'priority' were assigned jointly as 'foreground'
     * or 'background'. */
    bool m_useSimpleType;

    /* Activity is expected to run for a potentially indefinite anount of time.
     * Do not defer scheduling other things.  Activity should be set for
     * immediate scheduling, otherwise, it's a bug. */
    bool m_continuous;

    /* Activity was directly user-initiated. */
    bool m_userInitiated;

    /* Activity should support power debouncing */
    bool m_powerDebounce;

    /* Activity has been told to schedule itself by the Activity Manager */
    bool m_scheduled;

    /* Activity has requested permission to run */
    bool m_ready;

    /* Activity has started (prereqs met, dequeued if subject to queuing) */
    bool m_running;

    /* Activity is ending (by command or abandonment) */
    bool m_ending;

    /* Activity is ending due to explicit command */
    bool m_terminate;

    /* Activity has been requested to restart (via Complete) */
    bool m_restart;

    /* Activity had an issue while attempting to start, and should return to
     * the queue.  (Don't advance the Schedule, and don't lose the Trigger
     * information) */
    bool m_requeue;

    /* Activity has been told to yield. */
    bool m_yielding;

    bool m_released;

    std::shared_ptr<AbstractActivityState> m_state;

    AdopterQueue m_adopters;
    SubscriptionSet m_subscriptions;
    SubscriberSet m_subscribers;

    std::map<std::string, std::shared_ptr<IRequirement>> m_requirements;

    std::weak_ptr<AbstractSubscription> m_parent;
    std::weak_ptr<AbstractSubscription> m_releasedParent;

    TriggerConditionType m_triggerMode;
    std::vector<std::shared_ptr<ITrigger>> m_triggerVector;

    std::shared_ptr<AbstractCallback> m_callback;
    std::shared_ptr<Schedule> m_schedule;

    std::shared_ptr<PersistToken> m_persistToken;

    /* Activity Manager should arbitrate with Power Management to keep device
     * awake while Activity is running. */
    std::shared_ptr<AbstractPowerActivity> m_powerActivity;

    /* Index this in table of Activity names (and auto-unlink) */
    ActivityTableItem m_nameTableItem;

    /* Index this in table of Activity IDs (and auto-unlink).  This includes
     * Activities that have been released but still have references. */
    ActivityTableItem m_idTableItem;

    /* Start/Ready/Run queue link */
    ActivityListItem m_runQueueItem;

    /* List of pending Persist commands */
    CommandQueue m_persistCommands;

    double m_restartTime;
    int m_restartCount;

    MojRefCountedPtr<MojServiceMessage> m_msgForStart;
};

#endif /* __ACTIVITY_H__ */
