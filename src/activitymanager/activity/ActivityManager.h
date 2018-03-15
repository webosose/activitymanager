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

#ifndef __ACTIVITY_MANAGER_H__
#define __ACTIVITY_MANAGER_H__

#include <map>
#include <vector>

#include "Main.h"
#include "activity/Activity.h"
#include "base/Subscriber.h"
#include "base/Timeout.h"

/*
 * Central Activity registry and control object.
 */

class ActivityManager {
public:
    static ActivityManager& getInstance()
    {
        static ActivityManager _instance;
        return _instance;
    }
    virtual ~ActivityManager();

    typedef std::map<activityId_t, std::shared_ptr<Activity>> ActivityMap;
    typedef std::vector<std::shared_ptr<const Activity>> ActivityVec;

    void registerActivityId(std::shared_ptr<Activity> act);
    void registerActivityName(std::shared_ptr<Activity> act);
    void unregisterActivityName(std::shared_ptr<Activity> act);

    std::shared_ptr<Activity> getActivity(activityId_t id);
    std::shared_ptr<Activity> getActivity(const std::string& name, const BusId& creator);

    std::shared_ptr<Activity> getNewActivity(bool continuous);
    std::shared_ptr<Activity> getNewActivity(activityId_t id, bool continuous);

    void releaseActivity(std::shared_ptr<Activity> act);

    ActivityVec getActivities() const;

    /* Activity Commands Interface */
    MojErr startActivity(std::shared_ptr<Activity> act, MojServiceMessage *msg = NULL);
    MojErr stopActivity(std::shared_ptr<Activity> act);
    MojErr cancelActivity(std::shared_ptr<Activity> act);
    MojErr pauseActivity(std::shared_ptr<Activity> act);

    /* Activity Manager Control Interface */
    static const unsigned kExternalEnable = 0x1;
    static const unsigned kUIEnable = 0x2;
    static const unsigned kConfigurationLoaded = 0x4;
    static const unsigned kServiceOnline = 0x8;

    static const unsigned kEnableMask = kExternalEnable | kConfigurationLoaded | kServiceOnline;

    void enable(unsigned mask);
    void disable(unsigned mask);

    bool isEnabled() const;

    /* INTERFACE:  ACTIVITY ----> ACTIVITY MANAGER */

    /* The Activity is fully initialized and ready to be scheduled when
     * Activity Manager requests that it do so */
    void informActivityInitialized(std::shared_ptr<Activity> act);

    /* The Activity's prerequisites have been met, and it's ready to run
     * when the Activity Manager selects it to do so */
    void informActivityReady(std::shared_ptr<Activity> act);

    /* The Activity's prerequisites are not longer met.  It should be
     * returned to the waiting queue */
    void informActivityNotReady(std::shared_ptr<Activity> act);

    /* The Activity has started running */
    void informActivityRunning(std::shared_ptr<Activity> act);

    /* The Activity has begun the process of ending and subscribers have
     * been informed to unsubscribe */
    void informActivityEnding(std::shared_ptr<Activity> act);

    /* The Activity has ended.  If it wishes to restart, it will inform the
     * Activity Manager it's reinitialized again */
    void informActivityEnd(std::shared_ptr<Activity> act);

    /* The Activity has gained or lost a unique subscriber (BusId). */
    void informActivityGainedSubscriberId(std::shared_ptr<Activity> act, const BusId& id);
    void informActivityLostSubscriberId(std::shared_ptr<Activity> act, const BusId& id);

    /* END INTERFACE  */

    /* Activity Manager info state gatherer */
    MojErr infoToJson(MojObject& rep) const;

    static const char* const kClientName;
    static const char* const kServiceName;

    static const unsigned kDefaultBackgroundConcurrencyLevel = 1;
    static const unsigned kDefaultBackgroundInteractiveConcurrencyLevel = 2;
    static const unsigned kUnlimitedBackgroundConcurrency = 0;
    static const unsigned kDefaultBackgroundInteractiveYieldSeconds = 60;

private:
    ActivityManager();
    ActivityManager(const ActivityManager& copy);
    ActivityManager& operator=(const ActivityManager& copy);

protected:
    static gboolean _startActivity(gpointer data);
    void scheduleAllActivities();
    void evictQueue(std::shared_ptr<Activity> act);
    void runActivity(Activity& act);
    void runReadyBackgroundActivity(Activity& act);
    void runReadyBackgroundInteractiveActivity(Activity& act);
    unsigned getRunningBackgroundActivitiesCount() const;
    void checkReadyQueue();

    void updateYieldTimeout();
    void cancelYieldTimeout();
    void interactiveYieldTimeout();

protected:
    typedef enum {
        RunQueueNone = -1,
        RunQueueInitialized = 0,
        RunQueueScheduled,
        RunQueueReady,
        RunQueueReadyInteractive,
        RunQueueBackground,
        RunQueueBackgroundInteractive,
        RunQueueImmediate,
        RunQueueEnded,
        RunQueueMax
    } RunQueueId;

    /* Lightweight comparator object to search the Activity Name Table for
     * a particular name */
    typedef std::pair<std::string, BusId> ActivityKey;
    struct ActivityNameComp {
        bool operator()(const Activity& act1, const Activity& act2) const;
        bool operator()(const ActivityKey& key, const Activity& act) const;
        bool operator()(const Activity& act, const ActivityKey& key) const;
    };

    struct ActivityNameOnlyComp {
        bool operator()(const ActivityKey& key, const Activity& act) const;
        bool operator()(const Activity& act, const ActivityKey& key) const;
    };

    typedef boost::intrusive::member_hook<Activity, Activity::ActivityTableItem,
            &Activity::m_nameTableItem> ActivityNameTableOption;
    typedef boost::intrusive::set<Activity, ActivityNameTableOption,
            boost::intrusive::constant_time_size<false>,
            boost::intrusive::compare<ActivityNameComp> > ActivityNameTable;

    /* Comparator object for the Activity Id Table */
    struct ActivityIdComp {
        bool operator()(const Activity& act1, const Activity& act2) const;
        bool operator()(const activityId_t& id, const Activity& act) const;
        bool operator()(const Activity& act, const activityId_t& id) const;
    };

    typedef boost::intrusive::member_hook<Activity, Activity::ActivityTableItem,
            &Activity::m_idTableItem> ActivityIdTableOption;
    typedef boost::intrusive::multiset<Activity, ActivityIdTableOption,
            boost::intrusive::constant_time_size<false>,
            boost::intrusive::compare<ActivityIdComp>> ActivityIdTable;

    typedef boost::intrusive::member_hook<Activity, Activity::ActivityListItem,
            &Activity::m_runQueueItem> ActivityRunQueueOption;
    typedef boost::intrusive::list<Activity, ActivityRunQueueOption,
            boost::intrusive::constant_time_size<false>> ActivityRunQueue;

    static const char *kRunQueueNames[];

    /* Activity Name Table
     * The most recent Activity to attempt to claim a name will be listed
     * in the Table, and all Activties in the table are currently live and
     * not ending.  Activities should be removed from the table as soon as
     * they enter an ending state through cancel, stop, or complete (if it
     * isn't going to restart the Activity). */
    ActivityNameTable m_nameTable;

    /* Activity Id Table
     * Tracks currently instantiated Activities by Id.  This is slgihtly
     * different from the main Activity map, which tracks Activities that are
     * live.  This includes Activities that are still in the process of
     * being torn down.  (And will also enable automated tracking of Activities
     * which were otherwise leaked). */
    ActivityIdTable m_idTable;

    /* Activity Run Queues
     * Start Queue: Activities may wait here if the Activity Manager isn't
     *     prepared to schedule the Activity (generally, just on startup)
     * Ready Queue: Background Activities that are ready to run, but haven't
     *     gotten permission to do so yet
     * Ready Interactive Queue: Ready Background Activities initiated by the
     *     user
     * Background Run Queue: Background Activities that are currently running
     * Background Interactive Run Queue: Background Interactive Activities
     *     that are currently running
     * Immediate Run Queue: Immediate Activities that are currently running
     */
    ActivityRunQueue m_runQueue[RunQueueMax];

    /* Background Interactive Queue yield timeout */
    std::shared_ptr<TimeoutPtr<ActivityManager> > m_interactiveYieldTimeout;

    ActivityMap m_activities;

    unsigned m_enabled;

    unsigned m_backgroundConcurrencyLevel;
    unsigned m_backgroundInteractiveConcurrencyLevel;

    unsigned m_yieldTimeoutSeconds;

    activityId_t m_nextActivityId;
};

#endif /* __ACTIVITY_MANAGER_H__ */
