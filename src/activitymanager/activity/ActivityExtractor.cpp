// Copyright (c) 2009-2021 LG Electronics, Inc.
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
#include "ActivityExtractor.h"

#include <stdexcept>
#include <ctime>

#include "activity/callback/ActivityCallback.h"
#include "activity/schedule/IntervalSchedule.h"
#include "service/BusConnection.h"
#include "util/Logging.h"

std::shared_ptr<Activity> ActivityExtractor::createActivity(const MojObject& spec, bool reload)
{
    MojString activityName;
    MojString activityDescription;
    MojString metadataJson;

    MojUInt64 id;

    MojObject creator;
    MojObject metadata;
    MojObject typeSpec;
    MojObject requirementSpec;
    MojObject triggerSpec;
    MojObject callbackSpec;
    MojObject scheduleSpec;

    MojErr err;
    std::shared_ptr<Activity> act;
    bool found = false;
    bool continuous = false;

    err = spec.getRequired(_T("name"), activityName);
    if (err)
        throw std::runtime_error("Activity name is required");

    err = spec.getRequired(_T("description"), activityDescription);
    if (err)
        throw std::runtime_error("Activity description is required");

    if (spec.get(_T("type"), typeSpec)) {
        (void) typeSpec.get(_T("continuous"), continuous);
    }

    if (reload) {
        err = spec.get(_T("activityId"), id, found);
        if (err || !found) {
            throw std::runtime_error(
                    "If reloading Activities, activityId must be present");
        }

        found = spec.get(_T("creator"), creator);
        if (!found) {
            throw std::runtime_error(
                    "If reloading Activities, creator must be present");
        }

        act = ActivityManager::getInstance().getNewActivity((activityId_t) id, continuous);

        try {
            act->setCreator(processBusId(creator));
        } catch (...) {
            LOG_AM_ERROR(MSGID_DECODE_CREATOR_ID_FAILED, 2,
                         PMLOGKFV("activity", "%llu", id),
                         PMLOGKS("creator_id", MojoObjectJson(creator).c_str()),
                         "Unable to decode creator id while reloading");
            ActivityManager::getInstance().releaseActivity(std::move(act));
            throw;
        }
    } else {
        act = ActivityManager::getInstance().getNewActivity(continuous);
    }

    MojString reqExe;
    err = spec.get(_T("requesterExeName"),reqExe,found);

    if(!err && found)
    {
        act->setRequesterExeName(reqExe.data());
    }

    try {
        /* See if there's a private bus access forcing the creator.  If
         * reloading, the creator is already set (see above). */
        if (!reload) {
            bool found = spec.get(_T("creator"), creator);
            if (found) {
                act->setCreator(processBusId(creator));
            }
        }

        act->setName(activityName.data());
        act->setDescription(activityDescription.data());

        if (spec.get(_T("metadata"), metadata)) {
            if (metadata.type() != MojObject::TypeObject) {
                throw std::runtime_error(
                        "Object metadata should be set to a JSON object");
            }

            err = metadata.toJson(metadataJson);
            if (err) {
                throw std::runtime_error(
                        "Failed to convert provided Activity metadata object to JSON string");
            }

            act->setMetadata(metadataJson.data());
        }

        if (spec.get(_T("type"), typeSpec)) {
            processTypeProperty(act, typeSpec);
        }

        if (spec.get(_T("requirements"), requirementSpec)) {
            RequirementList addedRequirements;
            RequirementNameList removedRequirements;
            processRequirements(act, requirementSpec, removedRequirements,
                                addedRequirements);
            updateRequirements(act, removedRequirements, addedRequirements);
        }

        if (spec.get(_T("trigger"), triggerSpec)) {
            act->setTriggerVector(createTriggers(act, triggerSpec));
        }

        if (spec.get(_T("callback"), callbackSpec)) {
            std::shared_ptr<AbstractCallback> callback = createCallback(
                    act, callbackSpec);
            act->setCallback(callback);
        }

        if (spec.get(_T("schedule"), scheduleSpec)) {
            std::shared_ptr<Schedule> schedule = createSchedule(act,
                                                                scheduleSpec);
            act->setSchedule(schedule);
        }
    } catch (const std::exception& except) {
        LOG_AM_ERROR(MSGID_EXCEPTION_IN_RELOADING_ACTIVITY, 2,
                     PMLOGKFV("activity", "%llu", act->getId()),
                     PMLOGKS("Exception", except.what()),
                     "Unexpected exception reloading Activity from: '%s'",
                     MojoObjectJson(spec).c_str());
        ActivityManager::getInstance().releaseActivity(std::move(act));
        throw;
    } catch (...) {
        LOG_AM_ERROR(MSGID_RELOAD_ACTVTY_UNKNWN_EXCPTN, 1,
                     PMLOGKFV("activity","%llu",act->getId()),
                     "Unknown exception reloading Activity from: %s",
                     MojoObjectJson(spec).c_str());
        ActivityManager::getInstance().releaseActivity(std::move(act));
        throw;
    }

    return act;
}

void ActivityExtractor::updateActivity(std::shared_ptr<Activity> act,
                                       const MojObject& spec)
{
    /* First, parse and create the update, so it can be applied "atomically"
     * Then set or reset the Activity sections, as appropriate */
    MojObject triggerSpec;
    MojObject scheduleSpec;
    MojObject callbackSpec;
    MojObject metadata;
    MojObject requirements;

    MojString metadataJson;

    RequirementList addedRequirements;
    RequirementNameList removedRequirements;

    std::vector<std::shared_ptr<ITrigger>> triggerVec;
    std::shared_ptr<Schedule> schedule;
    std::shared_ptr<AbstractCallback> callback;

    bool clearTrigger = false;
    bool clearSchedule = false;
    bool clearMetadata = false;
    bool setMetadata = false;

    if (spec.get(_T("trigger"), triggerSpec)) {
        if (triggerSpec.type() == MojObject::TypeBool) {
            if (!triggerSpec.boolValue()) {
                clearTrigger = true;
            } else {
                throw std::runtime_error(
                        "\"trigger\":false to clear, or valid trigger property must be specified");
            }
        } else {
            triggerVec = createTriggers(act, triggerSpec);
        }
    }


    if (spec.get(_T("schedule"), scheduleSpec)) {
        if (scheduleSpec.type() == MojObject::TypeBool) {
            if (!scheduleSpec.boolValue()) {
                clearSchedule = true;
            } else {
                throw std::runtime_error(
                        "\"schedule\":false to clear, or valid schedule property must be specified");
            }
        } else {
            schedule = createSchedule(act, scheduleSpec);
        }
    }


    if (spec.get(_T("callback"), callbackSpec)) {
        if (callbackSpec.type() == MojObject::TypeBool) {
            if (!callbackSpec.boolValue()) {
                LOG_AM_ERROR(MSGID_RM_ACTVTY_CB_ATTEMPT, 1,
                             PMLOGKFV("activity","%llu",act->getId()),
                             "Attempt to remove callback on Complete");
                throw std::runtime_error(
                        "Activity callback may not be removed by Complete");
            }
        } else {
            callback = createCallback(act, callbackSpec);
        }
    }

    /* Allow Activity Metadata to be updated on 'complete'. */
    if (spec.get(_T("metadata"), metadata)) {
        if (metadata.type() == MojObject::TypeObject) {
            setMetadata = true;
            MojErr err = metadata.toJson(metadataJson);
            if (err) {
                LOG_AM_ERROR(
                        MSGID_DECODE_METADATA_FAIL,
                        1,
                        PMLOGKFV("activity","%llu",act->getId()),
                        "Failed to decode metadata to JSON while attempting to update");
                throw std::runtime_error("Failed to decode metadata to json");
            }
        } else if (metadata.type() == MojObject::TypeBool) {
            if (metadata.boolValue()) {
                LOG_AM_ERROR(MSGID_METADATA_UPDATE_TO_NONOBJ_TYPE, 1,
                             PMLOGKFV("activity","%llu",act->getId()),
                             "Attempt to update metadata to non-object type");
                throw std::runtime_error(
                        "Attempt to set metadata to a non-object type");
            } else {
                clearMetadata = true;
            }
        } else {
            LOG_AM_ERROR(MSGID_METADATA_UPDATE_TO_NONOBJ_TYPE, 1,
                         PMLOGKFV("activity","%llu",act->getId()),
                         "Attempt to update metadata to non-object type");
            throw std::runtime_error(
                    "Activity metadata must be set to an object if present");
        }
    }

    if (spec.get(_T("requirements"), requirements)) {
        processRequirements(act, requirements, removedRequirements,
                            addedRequirements);
    }

    if (clearTrigger) {
        act->clearTrigger();
    } else if (!triggerVec.empty()) {
        act->setTriggerVector(triggerVec);
    }

    if (clearSchedule) {
        act->clearSchedule();
    } else if (schedule) {
        act->setSchedule(schedule);
    }

    if (callback) {
        act->setCallback(callback);
    }

    if (setMetadata) {
        act->setMetadata(metadataJson.data());
    } else if (clearMetadata) {
        act->clearMetadata();
    }

    if (!addedRequirements.empty() || !removedRequirements.empty()) {
        updateRequirements(act, removedRequirements, addedRequirements);
    }
}

std::shared_ptr<Activity> ActivityExtractor::lookupActivity(
        const MojObject& payload, const BusId& caller)
{
    std::shared_ptr<Activity> act;
    MojString name;

    MojUInt64 id;
    bool found = false;
    MojErr err = payload.get(_T("activityId"), id, found);

    if (err) {
        throw std::runtime_error(
                "Error retrieving activityId of Activity to operate on");
    } else if (found) {
        act = ActivityManager::getInstance().getActivity((activityId_t) id);
    } else {

        err = payload.get(_T("activityName"), name, found);
        if (err != MojErrNone) {
            throw std::runtime_error(
                    "Error retrieving activityName of Activity to operate on");
        } else if (!found) {
            throw std::runtime_error(
                    "Activity ID or name not present in request");
        }

        act = ActivityManager::getInstance().getActivity(name.data(), caller);
    }

    return act;
}

std::vector<std::shared_ptr<ITrigger>> ActivityExtractor::createTriggers(
        std::shared_ptr<Activity> act, const MojObject& spec)
{
    MojObject multiTriggerSpec;

    LOG_AM_TRACE();

    std::vector<std::shared_ptr<ITrigger>> vec;

    if (spec.contains(_T("and"))) {
        act->setTriggerMode(TriggerConditionType::kMultiTriggerAndMode);
        spec.get(_T("and"), multiTriggerSpec);
    } else if (spec.contains(_T("or"))) {
        act->setTriggerMode(TriggerConditionType::kMultiTriggerOrMode);
        spec.get(_T("or"), multiTriggerSpec);
    }

    if (act->getTriggerMode() == TriggerConditionType::kMultiTriggerNone) {
        std::shared_ptr<ITrigger> trigger = createTrigger(std::move(act), spec);
        vec.push_back(std::move(trigger));
        return vec;
    }

    LOG_AM_DEBUG("multi trigger mode = %d", act->getTriggerMode());
    if (multiTriggerSpec.type() != MojObject::TypeArray) {
        LOG_AM_WARNING(MSGID_CREATE_ACTIVITY_EXCEPTION, 0,
                       "Multiple trigger required, but it is not");
        return vec;
    }

    for (MojObject::ConstArrayIterator iter = multiTriggerSpec.arrayBegin() ;
            iter != multiTriggerSpec.arrayEnd() ; ++iter) {
        std::shared_ptr<ITrigger> trigger = createTrigger(act, *iter);
        vec.push_back(std::move(trigger));
    }

    return vec;
}

std::shared_ptr<ITrigger> ActivityExtractor::createTrigger(std::shared_ptr<Activity> activity,
                                                           const MojObject& spec)
{
    MojErr err;

    std::shared_ptr<ITrigger> trigger;

    MojObject params;
    MojObject where;
    MojObject compare;
    MojObject value;

    MojString key;
    MojString method;

    err = spec.getRequired(_T("method"), method);
    if (err) {
        throw std::runtime_error("Method URL for Trigger is required");
    }

    LunaURL url(method);

    /* If no params, no problem. */
    spec.get(_T("params"), params);

    if (spec.contains(_T("where"))) {
        spec.get(_T("where"), where);

        trigger = TriggerFactory::createWhereTrigger(activity, url, params, where);
    } else if (spec.contains(_T("compare"))) {
        spec.get(_T("compare"), compare);

        if (!compare.contains(_T("key")) || !compare.contains(_T("value"))) {
            throw std::runtime_error(
                    "Compare Trigger requires key to compare and value"
                    " to compare against be specified.");
        }


        compare.getRequired(_T("key"), key);
        compare.get(_T("value"), value);

        trigger = TriggerFactory::createCompareTrigger(activity, url, params, key, value);
    } else if (spec.contains(_T("key"))) {
        spec.getRequired(_T("key"), key);

        trigger = TriggerFactory::createKeyedTrigger(activity, url, params, key);
    } else {
        trigger = TriggerFactory::createBasicTrigger(activity, url, params);
    }

    MojObject nameObj;
    MojString nameStr;
    if (spec.get(_T("name"), nameObj) &&
            nameObj.type() == MojObject::TypeString &&
            nameObj.stringValue(nameStr) == MojErrNone) {
        trigger->setName(nameStr.data());
    }

    return trigger;
}

std::shared_ptr<AbstractCallback> ActivityExtractor::createCallback(std::shared_ptr<Activity> activity,
                                                                    const MojObject& spec)
{
    MojErr err;

    MojString method;
    MojObject params;
    bool ignoreReturnValue = activity->isContinuous() ? true : false;

    err = spec.getRequired(_T("method"), method);
    if (err) {
        throw std::runtime_error("Method URL for Callback is required");
    }

    LunaURL url(method);

    /* If no params, no problem. */
    spec.get(_T("params"), params);

    MojObject ignoreObj;
    if (spec.get(_T("ignoreReturn"), ignoreObj) && ignoreObj.type() == MojObject::TypeBool) {
        ignoreReturnValue = ignoreObj.boolValue();
    }

    std::shared_ptr<AbstractCallback> callback = std::make_shared<
            ActivityCallback>(activity, url, params, ignoreReturnValue);

    return callback;
}

std::shared_ptr<Schedule> ActivityExtractor::createSchedule(
        std::shared_ptr<Activity> activity, const MojObject& spec)
{
    MojErr err;

    MojString startTimeStr;
    MojString intervalStr;
    MojString lastFinishedStr;

    bool found = false;
    bool startIsUTC = true;
    bool endIsUTC = true;
    bool precise = false;
    bool relative = false;
    bool skip = false;
    bool lastFinishedIsUTC = true;
    bool local;

    std::shared_ptr<Schedule> schedule;
    time_t startTime = IntervalSchedule::kDayOne;
    time_t endTime = Schedule::kUnbounded;

    err = spec.get(_T("start"), startTimeStr, found);
    if (err) {
        throw std::runtime_error("Failed to retreive start time from JSON");
    }

    if (found) {
        startTime = AbstractScheduleManager::stringToTime(startTimeStr.data(), startIsUTC);
    }

    found = false;
    err = spec.get(_T("interval"), intervalStr, found);
    if (err) {
        throw std::runtime_error("Interval must be specified as a string");
    }


    if (found) {
        found = false;
        MojString endTimeStr;
        err = spec.get(_T("end"), endTimeStr, found);
        if (err) {
            throw std::runtime_error("Error decoding end time string from "
                                     "schedule specification");
        } else if (found) {
            endTime = AbstractScheduleManager::stringToTime(endTimeStr.data(), endIsUTC);
        }

        spec.get(_T("precise"), precise);

        spec.get(_T("relative"), relative);

        spec.get(_T("skip"), skip);

        unsigned interval = IntervalSchedule::stringToInterval(
                intervalStr.data(), !precise);

        if (!precise && ((startTime != Schedule::kDayOne)
                || (endTime != Schedule::kUnbounded))) {
            throw std::runtime_error(
                    "Unless precise time is specified, time intervals"
                    " may not specify a start or end time");
        }

        std::shared_ptr<IntervalSchedule> intervalSchedule;

        if (!precise && relative) {
            throw std::runtime_error(
                    "An interval schedule can be specified as normal, precise, or precise and relative.");
        } else if (relative) {
            intervalSchedule = std::make_shared<RelativeIntervalSchedule>(
                    activity, startTime, interval, endTime);
        } else if (precise) {
            intervalSchedule = std::make_shared<PreciseIntervalSchedule>(
                    activity, startTime, interval, endTime);
        } else {
            intervalSchedule = std::make_shared<IntervalSchedule>(activity,
                                                                  startTime,
                                                                  interval,
                                                                  endTime);
        }

        if (skip) {
            intervalSchedule->setSkip(skip);
        }

        bool isUTC = false;

        if ((startTime != Schedule::kDayOne) && (endTime != Schedule::kUnbounded)) {
            if (startIsUTC != endIsUTC) {
                throw std::runtime_error(
                        "Start and end time must both be specified in UTC "
                        "or local time");
            }

            isUTC = startIsUTC;
        } else if (startTime != Schedule::kDayOne) {
            isUTC = startIsUTC;
        } else if (endTime != Schedule::kUnbounded) {
            isUTC = endIsUTC;
        }

        if (!isUTC) {
            intervalSchedule->setLocal(!isUTC);
        }

        found = false;
        err = spec.get(_T("lastFinished"), lastFinishedStr, found);
        if (err) {
            throw std::runtime_error(
                    "Error decoding end time string from schedule specification");
        } else if (found) {
            time_t lastFinishedTime = AbstractScheduleManager::stringToTime(lastFinishedStr.data(), lastFinishedIsUTC);
            if (isUTC != lastFinishedIsUTC) {
                LOG_AM_DEBUG(
                        "Last finished should use the same time format as the other times in the schedule");
            }
            intervalSchedule->setLastFinishedTime(lastFinishedTime);
        }

        schedule = intervalSchedule;
    } else {
        if (startTime == IntervalSchedule::kDayOne) {
            throw std::runtime_error(
                    "Non Interval Schedules must specify a start time");
        }

        schedule = std::make_shared<Schedule>(activity, startTime);

        if (!startIsUTC) {
            schedule->setLocal(true);
        }
    }

    found = spec.get(_T("local"), local);
    if (found) {
        schedule->setLocal(local);
    }

    return schedule;
}

void ActivityExtractor::processTypeProperty(std::shared_ptr<Activity> activity,
                                            const MojObject& type)
{
    bool found;
    bool persist = false;
    bool expl = false;
    bool continuous;
    bool userInitiated;
    bool powerActivity;
    bool powerDebounce;
    bool immediateSet = false;
    bool prioritySet = false;
    bool useSimpleType = false;
    bool immediate;
    bool foreground;
    bool background;

    MojErr err;
    MojString priorityStr;

    found = type.get(_T("persist"), persist);
    if (found) {
        activity->setPersistent(persist);
    }

    found = type.get(_T("explicit"), expl);
    if (found) {
        activity->setExplicit(expl);
    }

    found = type.get(_T("continuous"), continuous);
    if (found) {
        activity->setContinuous(continuous);
    }

    found = type.get(_T("userInitiated"), userInitiated);
    if (found) {
        activity->setUserInitiated(userInitiated);
    }

    found = type.get(_T("power"), powerActivity);
    if (found) {
        if (powerActivity) {
            /* The presence of a PowerActivity defines the
             * Activity as a power Activity. */
            activity->setPowerActivity(PowerManager::getInstance().createPowerActivity(activity));
        }
    }

    found = type.get(_T("powerDebounce"), powerDebounce);
    if (found) {
        activity->setPowerDebounce(powerDebounce);
    }

    /* Immediate can be set explicitly, or implied by foreground or
     * background.  It should only be set once, however.  Same with
     * priority.  Ultimately, you should *have* to set them, either
     * implicitly by setting foreground or background, or explicitly
     * by setting both. */

    ActivityPriority_t priority = ActivityPriorityNormal;

    found = type.get(_T("foreground"), foreground);
    if (found) {
        if (!foreground) {
            throw std::runtime_error("If present, 'foreground' should be specified as 'true'");
        }

        immediate = true;
        priority = ActivityForegroundPriority;
        immediateSet = true;
        prioritySet = true;
        useSimpleType = true;
    }

    found = type.get(_T("background"), background);
    if (found) {
        if (!background) {
            throw std::runtime_error("If present, 'background' should be specified as 'true'");
        }

        if (!immediateSet) {
            immediate = false;
            priority = ActivityBackgroundPriority;
            immediateSet = true;
            prioritySet = true;
            useSimpleType = true;
        } else {
            throw std::runtime_error("Only one of 'foreground' or 'background' should be specified");
        }
    }

    found = type.get(_T("immediate"), immediate);
    if (found) {
        if (!immediateSet) {
            immediateSet = true;
        } else {
            throw std::runtime_error("Only one of 'foreground', 'background', or 'immediate' should be specified");
        }
    }

    found = false;
    err = type.get(_T("priority"), priorityStr, found);
    if (err) {
        throw std::runtime_error("An error occurred attempting to access the 'priority' property of the type");
    } else if (found) {
        if (!prioritySet) {
            for (int i = 0 ; i < MaxActivityPriority ; i++) {
                if (priorityStr == ActivityPriorityNames[i]) {
                    priority = (ActivityPriority_t) i;
                    prioritySet = true;
                    break;
                }
            }

            if (!prioritySet) {
                throw std::runtime_error("Invalid priority specified");
            }
        } else {
            throw std::runtime_error("Only one of 'foreground', 'background', or 'priority' should be set");
        }
    }

    if (prioritySet) {
        activity->setPriority(priority);
    }

    if (immediateSet) {
        activity->setImmediate(immediate);
    }

    if (prioritySet || immediateSet) {
        activity->setUseSimpleType(useSimpleType);
    }
}

void ActivityExtractor::processRequirements(std::shared_ptr<Activity> activity,
                                            const MojObject& requirements,
                                            RequirementNameList& removedRequirements,
                                            RequirementList& addedRequirements)
{
    for (MojObject::ConstIterator iter = requirements.begin() ;
            iter != requirements.end() ; ++iter) {
        if ((iter.value().type() == MojObject::TypeNull) ||
                ((iter.value().type() == MojObject::TypeBool) && !iter.value().boolValue())) {
            removedRequirements.push_back(iter.key().data());
        } else {
            try {
                std::shared_ptr<IRequirement> req =
                        RequirementManager::getInstance().getRequirement(iter.key().data(), *iter);
                addedRequirements.push_back(std::move(req));
            } catch (...) {
                /* Really should pre-check the requirements to make sure
                 * the names are all valid, so we don't have to throw here. */
                throw;
            }
        }
    }
}

void ActivityExtractor::updateRequirements(std::shared_ptr<Activity> activity,
                                           RequirementNameList& removedRequirements,
                                           RequirementList& addedRequirements)
{
    /* Add requirements *FIRST*, then remove (so the Activity doesn't
     * pass through a state where it's runnable because it has no requirements
     * if what's happening is a requirement update). */
    for (RequirementList::iterator iter = addedRequirements.begin() ;
            iter != addedRequirements.end() ; ++iter) {
        activity->addRequirement(*iter);
    }

    for (RequirementNameList::iterator iter = removedRequirements.begin() ;
            iter != removedRequirements.end() ; ++iter) {
        activity->removeRequirement(*iter);
    }
}

BusId ActivityExtractor::processBusId(const MojObject& spec)
{
    if (spec.type() == MojObject::TypeObject) {
        if (spec.contains(_T("appId"))) {
            bool found = false;
            MojString appId;
            MojErr err = spec.get(_T("appId"), appId, found);
            if (err || !found) {
                throw std::runtime_error("Error getting appId string from object");
            }
            return BusId(appId.data(), BusApp);
        } else if (spec.contains(_T("serviceId"))) {
            bool found = false;
            MojString serviceId;
            MojErr err = spec.get(_T("serviceId"), serviceId, found);
            if (err || !found) {
                throw std::runtime_error("Error getting serviceId string from object");
            }
            return BusId(serviceId.data(), BusService);
        } else if (spec.contains(_T("anonId"))) {
            LOG_AM_WARNING(MSGID_PERSISTED_ANONID_FOUND, 0, "anonId subscriber found persisted in MojoDB");
            bool found = false;
            MojString anonId;
            MojErr err = spec.get(_T("anonId"), anonId, found);
            if (err || !found) {
                throw std::runtime_error("Error getting anonId string from object");
            }
            return BusId(anonId.data(), BusAnon);
        } else {
            throw std::runtime_error("Neither appId, serviceId, nor anonId was found in object");
        }
    } else if (spec.type() == MojObject::TypeString) {
        MojString creatorStr;

        MojErr err = spec.stringValue(creatorStr);
        if (err) {
            throw std::runtime_error("Failed to convert from simple object to string");
        }

        if (creatorStr.startsWith("appId:")) {
            return BusId(creatorStr.data() + 6, BusApp);
        } else if (creatorStr.startsWith("serviceId:")) {
            return BusId(creatorStr.data() + 10, BusService);
        } else if (creatorStr.startsWith("anonId:")) {
            return BusId(creatorStr.data() + 7, BusAnon);
        } else {
            throw std::runtime_error(
                    "BusId string did not start with \"appId:\", \"serviceId:\", or \"anonId:\"");
        }
    } else {
        throw std::runtime_error(
                "Could not convert to BusId - object is neither a new format object or old format string");
    }
}

