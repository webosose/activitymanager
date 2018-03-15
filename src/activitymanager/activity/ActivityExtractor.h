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

#ifndef __ACTIVITY_EXTRACTOR_H__
#define __ACTIVITY_EXTRACTOR_H__

#include <activity/schedule/AbstractScheduleManager.h>
#include <activity/trigger/TriggerFactory.h>
#include <core/MojObject.h>
#include <core/MojService.h>

#include "Main.h"

#include "activity/Activity.h"
#include "activity/ActivityManager.h"
#include "activity/schedule/Schedule.h"
#include "base/AbstractCallback.h"
#include "base/BusId.h"
#include "base/ITrigger.h"
#include "requirement/RequirementManager.h"

class ActivityExtractor {
public:
    virtual ~ActivityExtractor() {};

    static std::shared_ptr<Activity> createActivity(
            const MojObject& spec, bool reload = false);

    static void updateActivity(
            std::shared_ptr<Activity> activity, const MojObject& spec);

    static std::shared_ptr<Activity> lookupActivity(
            const MojObject& payload, const BusId& caller);

    static std::vector<std::shared_ptr<ITrigger>> createTriggers(
            std::shared_ptr<Activity> activity, const MojObject& spec);

    static std::shared_ptr<AbstractCallback> createCallback(
            std::shared_ptr<Activity> activity, const MojObject& spec);

private:
    ActivityExtractor() {};
    typedef std::list<std::string> RequirementNameList;
    typedef std::list<std::shared_ptr<IRequirement>> RequirementList;

    static BusId processBusId(const MojObject& spec);

    static std::shared_ptr<ITrigger> createTrigger(
            std::shared_ptr<Activity> activity, const MojObject& spec);

    static std::shared_ptr<Schedule> createSchedule(
            std::shared_ptr<Activity> activity, const MojObject& spec);

    static void processTypeProperty(std::shared_ptr<Activity> activity, const MojObject& type);

    static void processRequirements(std::shared_ptr<Activity> activity,
                             const MojObject& requirements,
                             RequirementNameList& removedRequirements,
                             RequirementList& addedRequirements);
    static void updateRequirements(std::shared_ptr<Activity> activity,
                            RequirementNameList& removedRequirements,
                            RequirementList& addedRequirements);

};

#endif /* __ACTIVITY_EXTRACTOR_H__ */
