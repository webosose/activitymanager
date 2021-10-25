// Copyright (c) 2009-2021 LG Electronics, Inc.
//
//      Copyright (c) 2009-2018 LG Electronics, Inc.
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
#include <db/DB8Command.h>
#include <db/ICompletion.h>
#include "activity/trigger/TriggerSubscription.h"
#include "ActivityCategoryHandler.h"

#include <stdexcept>

#include "Category.h"
#include "activity/state/AbstractActivityState.h"
#include "activity/callback/ActivityCallback.h"
#include "activity/type/AbstractPowerActivity.h"
#include "base/AbstractSubscription.h"
#include "conf/ActivityJson.h"
#include "conf/ActivityServiceSchemas.h"
#include "conf/Config.h"
#include "util/Logging.h"
#include "db/DB8Manager.h"

const MojChar* const ActivityCategoryHandler::CreateSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activity\" : {") ACTIVITY_TYPE_SCHEMA _T("}, ") \
            _T(" \"subscribe\" : { \"type\": \"boolean\", \"optional\": true } , ") \
            _T(" \"detailedEvents\" : { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"start\" : { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"replace\" : { \"type\": \"boolean\", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::ReleaseSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::MonitorSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"detailedEvents\" : { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"subscribe\": { \"type\": \"boolean\", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::JoinSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"subscribe\": { \"type\": \"boolean\" } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::PauseSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::AdoptSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"wait\": { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"subscribe\": { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"detailedEvents\" : { \"type\": \"boolean\", \"optional\" : true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::CompleteSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"restart\": { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"callback\": { ") CALLBACK_TYPE_SCHEMA _T(", \"optional\": true }, ") \
            _T(" \"schedule\": { ") SCHEDULE_TYPE_SCHEMA _T(", \"optional\": true }, ") \
            _T(" \"trigger\": { ") TRIGGER_TYPE_SCHEMA _T(", \"optional\": true }, ") \
            _T(" \"metadata\": { ") METADATA_TYPE_SCHEMA _T(", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::StartSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::StopSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::CancelSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::ListSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"details\": { \"type\": \"boolean\" }, ") \
            _T(" \"subscribers\": { \"type\": \"boolean\" }, ") \
            _T(" \"current\" : { \"type\": \"boolean\" }, ") \
            _T(" \"internal\" : { \"type\": \"boolean\" } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::GetDetailsSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"current\" : { \"type\": \"boolean\" }, ") \
            _T(" \"internal\" : { \"type\": \"boolean\" } ") \
        _T("}") \
    _T("}");

const MojChar* const ActivityCategoryHandler::GetActivityInfoSchema =
    _T("{ \"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"activityId\": { \"type\": \"integer\", \"optional\": true }, ") \
            _T(" \"activityName\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"subscribers\" : { \"type\": \"boolean\" }, ") \
            _T(" \"details\" : { \"type\": \"boolean\" }, ") \
            _T(" \"current\" : { \"type\": \"boolean\" }, ") \
            _T(" \"internal\" : { \"type\": \"boolean\" } ") \
        _T("}") \
    _T("}");

/*!
 * \page com_palm_activitymanager Service API com.palm.activitymanager/
 * Public methods:
 * - \ref com_palm_activitymanager_create
 * - \ref com_palm_activitymanager_release
 * - \ref com_palm_activitymanager_adopt
 * - \ref com_palm_activitymanager_complete
 * - \ref com_palm_activitymanager_start
 * - \ref com_palm_activitymanager_stop
 * - \ref com_palm_activitymanager_cancel
 *
 * Private methods:
 * - \ref com_palm_activitymanager_monitor
 * - \ref com_palm_activitymanager_join
 * - \ref com_palm_activitymanager_pause
 * - \ref com_palm_activitymanager_focus
 * - \ref com_palm_activitymanager_unfocus
 * - \ref com_palm_activitymanager_addfocus
 * - \ref com_palm_activitymanager_list
 * - \ref com_palm_activitymanager_get_details
 * - \ref com_palm_activitymanager_info
 * - \ref com_palm_activitymanager_enable
 * - \ref com_palm_activitymanager_disable
 */

const ActivityCategoryHandler::SchemaMethod ActivityCategoryHandler::s_methods[] = {
    { _T("create"), (Callback) &ActivityCategoryHandler::createActivity, ActivityCategoryHandler::CreateSchema },
    { _T("release"), (Callback) &ActivityCategoryHandler::releaseActivity, ActivityCategoryHandler::ReleaseSchema },
    { _T("adopt"), (Callback) &ActivityCategoryHandler::adoptActivity, ActivityCategoryHandler::AdoptSchema },
    { _T("complete"), (Callback) &ActivityCategoryHandler::completeActivity, ActivityCategoryHandler::CompleteSchema },
    { _T("start"), (Callback) &ActivityCategoryHandler::startActivity, ActivityCategoryHandler::StartSchema },
    { _T("stop"), (Callback) &ActivityCategoryHandler::stopActivity, ActivityCategoryHandler::StopSchema },
    { _T("cancel"), (Callback) &ActivityCategoryHandler::cancelActivity, ActivityCategoryHandler::CancelSchema },
    { _T("monitor"), (Callback) &ActivityCategoryHandler::monitorActivity, ActivityCategoryHandler::MonitorSchema },
    { _T("join"), (Callback) &ActivityCategoryHandler::joinActivity, ActivityCategoryHandler::JoinSchema },
    { _T("pause"), (Callback) &ActivityCategoryHandler::pauseActivity, ActivityCategoryHandler::PauseSchema },
    { _T("list"), (Callback) &ActivityCategoryHandler::listActivities, ActivityCategoryHandler::ListSchema },
    { _T("getDetails"), (Callback) &ActivityCategoryHandler::getActivityDetails, ActivityCategoryHandler::GetDetailsSchema },
    { _T("getActivityInfo"), (Callback) &ActivityCategoryHandler::getActivityInfo, ActivityCategoryHandler::GetActivityInfoSchema },
    { _T("info"), (Callback) &ActivityCategoryHandler::getManagerInfo, NULL },
    { _T("getManagerInfo"), (Callback) &ActivityCategoryHandler::getManagerInfo, NULL },
    { _T("addFocus"), (Callback) &ActivityCategoryHandler::replyDeprecatedMethod, NULL },
    { _T("focus"), (Callback) &ActivityCategoryHandler::replyDeprecatedMethod, NULL },
    { _T("unfocus"), (Callback) &ActivityCategoryHandler::replyDeprecatedMethod, NULL },
    { _T("disable"), (Callback) &ActivityCategoryHandler::replyDeprecatedMethod, NULL },
    { _T("enable"), (Callback) &ActivityCategoryHandler::replyDeprecatedMethod, NULL },
    { NULL, NULL, NULL }
};

ActivityCategoryHandler::ActivityCategoryHandler(
        std::shared_ptr<PermissionManager> pm)
    : m_pm(pm)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Constructing");
}

ActivityCategoryHandler::~ActivityCategoryHandler()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Destructing");
}

MojErr ActivityCategoryHandler::init()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Initializing methods");
    MojErr err;

    err = addMethods(s_methods);

    MojErrCheck(err);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_create create

\e Public.

com.palm.activitymanager/create

<b>Creates a new Activity and returns its ID.</b>

You can create either a foreground or background Activity. A foreground Activity
is run as soon as its specified prerequisites are met. After a background
Activity's prerequisites are met, it is moved into a ready queue, and a limited
number are allowed to run depending on system resources. Foreground is the
default.

Activities can be scheduled to run at a specific time or when certain conditions
are met or events occur.

Each of your created and owned Activities must have a unique name. To replace
one of your existing Activities, set the "replace" flag to true. This cancels
the original Activity and replaces it with the new Activity.

To keep an Activity alive and receive status updates, the parent (and adopters,
if any) must set the "subscribe" flag to true.Activities with a callback and
"subscribe=false", the Activity must be adopted immediately after the callback
is invoked for the Activity to continue.

To indicate the Activity is fully-initialized and ready to launch, set the
`"start"` flag to `true`. Activities with a callback should be started when
created - the callback is invoked when the prerequisites have been met and, in
the case of a background, non-immediate Activity, it has been cleared to run.

<b>When requirements are not initially met</b>

If the creator of the Activity also specifies "subscribe":true, and detailed
events are enabled for that subscription, then the Activity Manager will
generate either an immediate "start" event if the requirements are met, or an
"update" event if the Activity is not yet ready to start due to missing
requirements, schedule, or trigger. This allows the creating Service to
determine if it should continue executing while waiting for the callback,
or exit to free memory if it may be awhile before the Activity is ready to run.

\subsection com_palm_activitymanager_create_syntax Syntax:
\code
{
    "activity": Activity object,
    "subscribe": boolean,
    "detailedEvents": boolean,
    "start": boolean,
    "replace": boolean
}
\endcode

\param activity Activity object. \e Required.
\param subscribe Subscribe to Activity flag.
\param detailedEvents Flag to have the Activity Manager generate "update" events
       when the state of one of this Activity's requirements changes.
\param start Start Activity immediately flag.
\param replace Cancel Activity and replace with Activity of same name flag.

\subsection com_palm_activitymanager_create_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean,
    "subscribed": boolean,
    "activityId": int
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.
\param subscribed True if the call was subscribed.
\param activityId Activity ID.

\subsection com_palm_activitymanager_create_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/create '{ "activity": { "name": "basicactivity", "description": "Test create", "type": { "foreground": true } }, "start": true, "subscribe": true }'
\endcode

Example response for a succesful call, followed by an update event:
\code
{
    "activityId": 10813,
    "returnValue": true,
    "subscribed": true
}
{
    "activityId": 10813,
    "event": "start",
    "returnValue": true,
    "subscribed": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 22,
    "errorText": "Activity specification not present",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::createActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Create: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    MojObject spec;
    err = payload.getRequired(_T("activity"), spec);
    if (err) {
        err = msg->replyError(MojErrInvalidArg, "Activity specification not present");
        MojErrCheck(err);
        return MojErrNone;
    }

    /* TODO : Refine this, when ACG migration is complete.
     * Until then, we allow the creator is set only for the configurator. */
    MojObject creator;
    bool privilegedCreator = false;
    if (Subscription::getServiceName(msg) == "com.webos.service.configurator" ||
        Subscription::getServiceName(msg) == "com.palm.configurator")
        privilegedCreator = true;

    if (spec.get(_T("creator"), creator) && !privilegedCreator) {
        LOG_AM_ERROR(MSGID_PBUS_CALLER_SETTING_CREATOR, 2,
                     PMLOGKS("sender", Subscription::getServiceName(msg).c_str()),
                     PMLOGKS("creator", MojoObjectJson(creator).c_str()),
                     "Only configurator can set 'creator'");
        err = msg->replyError(MojErrInvalidArg, "Only configurator can set 'creator'");
        MojErrCheck(err);
        return MojErrNone;
    }

    /* First, parse and initialize the Activity.  No callouts to other
     * Services will be generated until the Activity is scheduled to Start.
     * It's very possible that the Create can fail (for various reasons)
     * later, and that's fine... when the Activity is released it will be
     * torn down. */
    std::shared_ptr<Activity> act;

    try {
        act = ActivityExtractor::createActivity(spec);

        /* If creator wasn't already set by the caller. */
        if (act->getCreator().getType() == BusNull) {
            act->setCreator(Subscription::getBusId(msg));
        }
    } catch (const std::exception& except) {
        err = msg->replyError(MojErrNoMem, except.what());
        MojErrCheck(err);
        return MojErrNone;
    }


    /* Next, check to make sure we can move forward with creating the Activity.
     * This essentially means:
     * - The creator is going to subscribe or the Activity is going to start
     *   now *and* has a callback (or else no one will know that it's
     *   starting).
     * - That no other Activity with the same name currently exists for this
     *   creator, unless "replace" has been specified.
     */
    bool subscribed = false;
    payload.get(_T("subscribe"), subscribed);

    bool start = false;
    payload.get(_T("start"), start);

    if (!subscribed && !(start && act->hasCallback())) {
        ActivityManager::getInstance().releaseActivity(act);

        err = msg->replyError(MojErrInvalidArg,
                "Created Activity must specify \"start\" and a Callback if not subscribed");
        MojErrCheck(err);
        return MojErrNone;
    }

    std::string requester = privilegedCreator ? Subscription::getServiceName(msg) : PermissionManager::getRequester(msg);
    std::string requesterExeName = PermissionManager::getRequesterExeName(msg);
    act->setRequesterExeName(requesterExeName);
    PermissionManager::PermissionCallback permissionCallback =
            std::bind(&ActivityCategoryHandler::finishCreateActivityPermissionCheck, this,
                      MojRefCountedPtr<MojServiceMessage>(msg), payload, act,
                      std::placeholders::_1, std::placeholders::_2);
    checkAccessRight(requester, act->getCallback(), act->getTriggers(),
                     permissionCallback);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

MojErr ActivityCategoryHandler::finishCreateActivityPermissionCheck(
        MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload, std::shared_ptr<Activity> act,
        MojErr errorCode, std::string errorText) {
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_DEBUG("Create permission: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 errorText.empty() ? "succeeded" : errorText.c_str());

    MojErr err = MojErrNone;

    if (errorCode) {
        err = msg->replyError(errorCode, errorText.c_str());
        MojErrCheck(err);
        return MojErrNone;
    }

    bool replace = false;
    payload.get(_T("replace"), replace);

    std::shared_ptr<Activity> old;
    try {
        old = ActivityManager::getInstance().getActivity(act->getName(), act->getCreator());
    } catch (...) {
        /* No Activity with that name/creator pair found. */
    }

    if (old && !replace) {
        LOG_AM_WARNING(MSGID_OLD_ACTIVITY_NO_REPLACE, 3,
                       PMLOGKS("activity_name",act->getName().c_str()),
                       PMLOGKS("creator_name",act->getCreator().getString().c_str()),
                       PMLOGKFV("old_activity","%llu",old->getId()), "");

        ActivityManager::getInstance().releaseActivity(act);
        err = msg->replyError(MojErrExists, _T("Activity with that name already exists"));

        MojErrCheck(err);
        return MojErrNone;
    }

    /* "Paaaaaaast the point of no return.... no backward glaaaanceeesss!!!!!"
     *
     * At this point, failure to create the Activity is an exceptional event.
     * Persistence failures with MojoDB should be retried.  Ultimately this
     * could end up with a hung Activity (if events aren't generated), or
     * an Activity that ends up non-persisted.
     *
     * Stopping and starting the Activity Manager should return to a sane
     * state, although clients with non-persistent Activities may be unhappy.
     */

    if (old) {
        LOG_AM_DEBUG("[Activity %llu] Replace old activity %llu", act->getId(), old->getId());
        old->plugAllSubscriptions();
        /* XXX protect against this failing */
        old->getState()->destroy(old);
        if (old->isNameRegistered()) {
            ActivityManager::getInstance().unregisterActivityName(old);
        }
    }

    ActivityManager::getInstance().registerActivityId(act);
    ActivityManager::getInstance().registerActivityName(act);

    if (old) {
        ensureReplaceCompletion(msg.get(), payload, old, act);
        old->unplugAllSubscriptions();
    } else {
        ensureCompletion(msg.get(), payload, AbstractPersistManager::StoreCommandType,
                &ActivityCategoryHandler::finishCreateActivity, act);
    }

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

void ActivityCategoryHandler::checkAccessRight(const std::string& requester,
                                               std::shared_ptr<AbstractCallback> c,
                                               std::vector<std::shared_ptr<ITrigger>> vec,
                                               PermissionManager::PermissionCallback permissionCB)
{
    std::vector<std::string> urls;

    std::shared_ptr<ActivityCallback> callback = std::dynamic_pointer_cast<ActivityCallback>(c);
    if (callback) {
        urls.push_back(callback->getMethod().getString());
    }

    for (auto& trigger : vec) {
        std::shared_ptr<TriggerSubscription> subscription;
        if (trigger && (subscription = trigger->getSubscription())) {
            urls.push_back(subscription->getURL().getString());
        }
    }

    m_pm->checkAccessRight(requester, urls, permissionCB);
}

void ActivityCategoryHandler::finishCreateActivity(MojRefCountedPtr<MojServiceMessage> msg,
                                                   const MojObject& payload,
                                                   std::shared_ptr<Activity> act,
                                                   bool succeeded)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Create finishing: Message from %s: [Activity %llu]: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 act->getId(), succeeded ? "succeeded" : "failed");

    MojErr err = MojErrNone;

    bool start = false;
    payload.get(_T("start"), start);

    std::shared_ptr<Subscription> sub;
    bool subscribed = subscribeActivity(msg.get(), payload, act, sub);
    if (subscribed) {
        act->setParent(sub);
    }

    LOG_AM_DEBUG("Create complete: [Activity %llu] (\"%s\") created by %s",
                 act->getId(), act->getName().c_str(),
                 Subscription::getSubscriberString(msg).c_str());

    if (start) {
        ActivityManager::getInstance().startActivity(act, msg.get());

        /* If an Activity isn't immediately running after being started,
         * generate an update event to explain what the holdup is to any
         * subscribers. */
        if (!act->isRunning()) {
            act->broadcastEvent(kActivityUpdateEvent);
        }

        return;
    }

    MojObject reply;

    err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
    MojErrGoto(err, fail);

    err = reply.putInt(_T("activityId"), (MojInt64) act->getId());
    MojErrGoto(err, fail);

    err = reply.putBool(_T("subscribed"), subscribed);
    MojErrGoto(err, fail);

    err = msg->reply(reply);
    MojErrGoto(err, fail);

    return;

fail:
    /* XXX Kill subscription */
    ActivityManager::getInstance().releaseActivity(act);

    ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

void ActivityCategoryHandler::finishReplaceActivity(std::shared_ptr<Activity> act, bool succeeded)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Replace finishing: Replace Activity [%llu]: %s",
                 act->getId(), succeeded ? "succeeded" : "failed");

    /* Release the Activity's Persist Token.  The Activity that replaced it
     * owns it now.  (This is really just a safety to make sure that any
     * further actions on the Activity can't accidentally affect the
     * persistence of the new Activity) */
    act->clearPersistToken();
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_join join

\e Public.

com.palm.activitymanager/join

Subscribe to receive events from a particular Activity.

\subsection com_palm_activitymanager_join_syntax Syntax:
\code
{
    "activityId": int,
    "subscribe": true
}
\endcode

\param activityId The Activity to join to.
\param subscribe Must be true for this call to succeed.

\subsection com_palm_activitymanager_join_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean,
    "subscribed": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.
\param subscribed True if the call was subscribed.

\subsection com_palm_activitymanager_join_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/join '{ "activityId": 81, "subscribe": true }'
\endcode

Example response for a succesful call followed by a status update event:
\code
{
    "returnValue": true,
    "subscribed": true
}
{
    "activityId": 81,
    "event": "stop",
    "returnValue": true,
    "subscribed": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 22,
    "errorText": "Join method calls must subscribe to the Activity",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::joinActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Join: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    bool subscribe = false;
    payload.get(_T("subscribe"), subscribe);
    if (!subscribe) {
        err = msg->replyError(MojErrInvalidArg, "Join method calls must subscribe to the Activity");
        MojErrCheck(err);
        return MojErrNone;
    }

    err = monitorActivity(msg, payload);
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_monitor monitor

\e Public.

com.palm.activitymanager/monitor

Given an activity ID, returns the current activity state. If the caller chooses
to subscribe, additional Activity status updates are returned as they occur.

\subsection com_palm_activitymanager_monitor_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "subscribe": boolean,
    "detailedEvents": boolean
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param subscribe Activity subscription flag. \e Required.
\param detailedEvents Flag to have the Activity Manager generate "update" events
       when the state of one of this Activity's requirements changes.
       \e Required.

\subsection com_palm_activitymanager_monitor_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean,
    "subscribed": boolean,
    "state": string
}

\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.
\param subscribed True if the call was subscribed.
\param state Activity state.

\subsection com_palm_activitymanager_monitor_examples Examples:
\code
luna-send -i -f  luna://com.palm.activitymanager/monitor '{ "activityId": 71, "subscribe": false, "detailedEvents": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "subscribed": false,
    "state": "running"
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "Error retrieving activityId of Activity to operate on",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::monitorActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Monitor: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);

    err = checkSerial(msg, payload, act);
    MojErrCheck(err);

    std::shared_ptr<Subscription> sub;
    bool subscribed = subscribeActivity(msg, payload, act, sub);

    MojObject reply;

    err = reply.putString(_T("state"), act->getState()->toString().c_str());
    MojErrCheck(err);

    err = reply.putBool(_T("subscribed"), subscribed);
    MojErrCheck(err);

    err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
    MojErrCheck(err);

    err = msg->reply(reply);
    MojErrCheck(err);

    if (subscribed) {
        LOG_AM_DEBUG("Monitor: %s subscribed to [Activity %llu]",
                     Subscription::getSubscriberString(msg).c_str(), act->getId());
    } else {
        LOG_AM_DEBUG("Monitor: Returned [Activity %llu] state to %s",
                     act->getId(), Subscription::getSubscriberString(msg).c_str());
    }

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_release release

\e Public.

com.palm.activitymanager/release

Allows a parent to free an Activity and notify other subscribers. The Activity
is cancelled unless one of its non-parent subscribers adopts it and becomes the
new parent. This has to happen in the timeout specified. If no timeout is
specified, the Activity is cancelled immediately. For a completely safe
transfer, a subscribing app or service, prior to the release, should already
have called adopt, indicating its willingness to take over as the parent.

\subsection com_palm_activitymanager_release_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "timeout": int
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.
\param timeout Time to wait, in seconds, for Activity to be adopted after release.

\subsection com_palm_activitymanager_release_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_release_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/release '{ "activityId": 2, "timeout": 30 }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::releaseActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Release: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);

    err = act->release(Subscription::getBusId(msg));
    if (err) {
        LOG_AM_DEBUG("Release: %s failed to release [Activity %llu]",
                     Subscription::getSubscriberString(msg).c_str(), act->getId());
    } else {
        LOG_AM_DEBUG("Release: [Activity %llu] successfully released by %s", act->getId(),
                     Subscription::getSubscriberString(msg).c_str());
    }

    MojErrCheck(err);

    err = msg->replySuccess();
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_adopt adopt

\e Public.

com.palm.activitymanager/adopt

Registers an app's or service's willingness to take over as the Activity's
parent.

If your app can wait for an unavailable Activity (not released) to become
available, then set the "wait" flag to true. If it is, and the Activity is valid
(exists and is not exiting), the call should succeed. If it cannot wait, and the
Activity is valid but cannot be adopted, then the call fails. The adopted return
flag indicates a successful or failed adoption.

If not immediately adopted and waiting is requested, the "orphan" event informs
the adopter that they are the new Activity parent.

An example where adoption makes sense is an app that allocates a separate
Activity for a sync, and passes that Activity to a service to use. The service
should adopt the Activity and be ready to take over in the event the app exits
before the service is done syncing. Otherwise, it receives a "cancel" event and
should exit immediately. The service should wait until the adopt (or monitor)
call returns successfully before beginning Activity work. If adopt or monitor
fails, it indicates the caller has quit or closed the Activity and the request
should not be processed. The Service should continue to process incoming events
on their subscription to the Activity.

If the service did not call adopt to indicate to the Activity Manager its
willingness to take over as the parent, it should be prepared to stop work for
the Activity and unsubscribe if it receives a "cancel" event. Otherwise, if it
receives the "orphan" event indicating the parent has unsubscribed and the
service is now the parent, then it should use complete when finished to inform
the Activity Manager before unsubscribing.

\subsection com_palm_activitymanager_adopt_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "wait": boolean,
    "subscribe": boolean,
    "detailedEvents": boolean
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param wait Wait for Activity to be released flag. \e Required.
\param subscribe Flag to subscribe to Activity and receive Activity events.
       \e Required.
\param detailedEvents Flag to have the Activity Manager generate "update" events
       when the state of an Activity's requirement changes. \e Required.

\subsection com_palm_activitymanager_adopt_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean,
    "subscribed": boolean,
    "adopted": boolean
}
\endcode


\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.
\param subscribed True if the call was subscribed.
\param adopted True if the Activity was adopted.

\subsection com_palm_activitymanager_adopt_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/adopt '{ "activityId": 876, "wait": true, "subscribe": true, "detailedEvents" : false }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "subscribed": true,
    "adopted": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::adoptActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Adopt: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                  MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;
    bool wait = false;
    bool adopted = false;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);

    err = checkSerial(msg, payload, act);
    MojErrCheck(err);

    std::shared_ptr<Subscription> sub;
    bool subscribed = subscribeActivity(msg, payload, act, sub);

    if (!subscribed) {
        err = msg->replyError(MojErrInvalidArg, "AdoptActivity requires subscription");
        MojErrCheck(err);
        return MojErrNone;
    }

    payload.get(_T("wait"), wait);

    err = act->adopt(sub, wait, &adopted);
    if (err) {
        LOG_AM_DEBUG("Adopt: %s failed to adopt [Activity %llu]",
                     sub->getSubscriber().getString().c_str(), act->getId());
    } else if (adopted) {
        LOG_AM_DEBUG("Adopt: [Activity %llu] adopted by %s",
                     act->getId(), sub->getSubscriber().getString().c_str());
    } else {
        LOG_AM_DEBUG("Adopt: %s queued to adopt [Activity %llu]",
                     sub->getSubscriber().getString().c_str(), act->getId());
    }

    MojErrCheck(err);

    MojObject reply;

    err = reply.putBool(_T("adopted"), adopted);
    MojErrCheck(err);

    err = reply.putBool(_T("subscribed"), subscribed);
    MojErrCheck(err);

    err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
    MojErrCheck(err);

    err = msg->reply(reply);
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_complete complete

\e Public.

com.palm.activitymanager/complete

An Activity's parent can use this method to end the Activity and optionally
restart it with new attributes. If there are other subscribers, they are sent a
"complete" event.

If restart is requested, the Activity is optionally updated with any new
Callback, Schedule, Requirements, or Trigger data and returned to the queued
state. Specifying false for any of those properties removes the property
completely. For any properties not specified, current properties are used.

If the Activity is persistent (specified with the persist flag in the Type
object), the db8 database is updated before the call returns.

\subsection com_palm_activitymanager_complete_syntax Syntax:
\code
{
    "activityId": int
    "activityName": string,
    "restart": boolean,
    "callback": false or Callback,
    "schedule": false or Schedule,
    "requirements": false or Requirements,
    "trigger": false or Triggers,
    "metadata: false or any object
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param restart Restart Activity flag. Default is false.
\param callback Callback to use if Activity is restarted.
\param schedule Schedule to use if Activity is restarted.
\param requirements Prerequisites to use if Activity is restarted.
\param trigger Trigger to use if Activity is restarted.
\param metadata Meta data.

\subsection com_palm_activitymanager_complete_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_complete_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/complete '{ "activityId": 876 }'
\endcode

Example response for a succesful call:
\code
{
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": true
}
\endcode
*/

MojErr ActivityCategoryHandler::completeActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Complete: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);

    err = checkSerial(msg, payload, act);
    MojErrCheck(err);

    bool restart = false;
    payload.get(_T("restart"), restart);

    /* Check the permission of activity creator & activitymanager */
    if (restart) {

        std::shared_ptr<AbstractCallback> callback;
        MojObject callbackSpec;
        if (payload.get(_T("callback"), callbackSpec))
        {
            if (callbackSpec.type() != MojObject::TypeBool)
                callback = ActivityExtractor::createCallback(act, callbackSpec);
        }

        std::shared_ptr<ActivityCallback> _new =
                std::dynamic_pointer_cast<ActivityCallback>(callback);

        std::shared_ptr<ActivityCallback> _old =
                std::dynamic_pointer_cast<ActivityCallback>(act->getCallback());

        bool callbackChanged =
                (_new && _old && _old->getMethod() != _new->getMethod()) || (!_old && _new);

        std::vector<std::shared_ptr<ITrigger>> triggerVec;
        MojObject triggerSpec;
        if (payload.get(_T("trigger"), triggerSpec)) {
            if (triggerSpec.type() != MojObject::TypeBool)
                triggerVec = ActivityExtractor::createTriggers(act, triggerSpec);
        }

        std::string requester = PermissionManager::getRequester(msg);
        PermissionManager::PermissionCallback permissionCallback =
                std::bind(&ActivityCategoryHandler::finishCompleteActivityPermissionCheck, this,
                          MojRefCountedPtr<MojServiceMessage>(msg), payload, act, restart,
                          std::placeholders::_1, std::placeholders::_2);
        checkAccessRight(requester,
                         (callbackChanged ? callback : std::shared_ptr<AbstractCallback>()),
                         triggerVec,
                         permissionCallback);

        return MojErrNone;
    }

    finishCompleteActivityPermissionCheck(MojRefCountedPtr<MojServiceMessage>(msg), payload,
                                          act, restart, MojErrNone, "");

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}


MojErr ActivityCategoryHandler::finishCompleteActivityPermissionCheck(
        MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload, std::shared_ptr<Activity> act,
        bool restart, MojErr errorCode, std::string errorText)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_DEBUG("Complete permission: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 errorText.empty() ? "succeeded" : errorText.c_str());

    MojErr err = MojErrNone;

    if (errorCode) {
        err = msg->replyError(errorCode, errorText.c_str());
        MojErrCheck(err);
        return MojErrNone;
    }

    /* XXX Only from private bus - for testing only */
    bool force = false;
    payload.get(_T("force"), force);

    if (restart) {
        /* XXX Technically, if complete fails, the Activity would be updated
         * anyways.  Which leads to a potentially odd condition.  Add a
         * "CheckComplete" command to check to see if the command can succeed?
         */
        ActivityExtractor::updateActivity(act, payload);
        act->setRestartFlag(true);
    } else {
        act->setTerminateFlag(true);
    }

    act->plugAllSubscriptions();

    /* XXX Catch and unplug */
    err = act->complete(Subscription::getBusId(msg), force);
    if (err == MojErrNone) {
        LOG_AM_DEBUG("Complete: %s completing [Activity %llu]",
                     Subscription::getSubscriberString(msg).c_str(), act->getId());

        ensureCompletion(
                msg.get(),
                payload,
                (restart ? AbstractPersistManager::StoreCommandType : AbstractPersistManager::DeleteCommandType),
                &ActivityCategoryHandler::finishCompleteActivity,
                act);

        act->unplugAllSubscriptions();
    } else {
        LOG_AM_DEBUG("Complete: %s failed to complete [Activity %llu]",
                     Subscription::getSubscriberString(msg).c_str(), act->getId());

        /* XXX Remove these once CheckComplete is in place */
        act->setTerminateFlag(false);
        act->unplugAllSubscriptions();

        msg->replyError(err);
    }

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

void ActivityCategoryHandler::finishCompleteActivity(MojRefCountedPtr<MojServiceMessage> msg,
                                                     const MojObject& payload,
                                                     std::shared_ptr<Activity> act,
                                                     bool succeeded)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Complete finishing: Message from %s: [Activity %llu]: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 act->getId(), succeeded ? "succeeded" : "failed");

    bool restart = false;
    payload.get(_T("restart"), restart);

    if (!restart && act->isNameRegistered()) {
        ActivityManager::getInstance().unregisterActivityName(act);
    }

    MojErr err = msg->replySuccess();
    if (err)
        LOG_AM_ERROR(MSGID_ACTVTY_REPLY_ERR, 0, "Failed to generate reply to Complete request");

    LOG_AM_DEBUG("Complete: %s completed [Activity %llu]",
                 Subscription::getSubscriberString(msg).c_str(), act->getId());

    ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_start start

\e Public.

com.palm.activitymanager/start

Attempts to start the specified Activity, either moving it from the "init" state
to be eligible to run, or resuming it if it is currently paused. This sends
"start" events to any subscribed listeners once the Activity is cleared to
begin. If the "force" parameter is present (and true), other Activities could be
cancelled to free resources the Activity needs to run.

\subsection com_palm_activitymanager_start_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "force": boolean
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.
\param force Force the Activity to run flag. If "true", other Activities could
             be cancelled to free resources the Activity needs to run.

\subsection com_palm_activitymanager_start_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_start_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/start '{ "activityId": 2, "force": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::startActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Start: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);
    if (!validateCaller(Subscription::getBusId(msg), act)) {
        err = msg->replyError(MojErrAccessDenied, "caller is not a parent or creator");
        MojErrCheck(err);
        return MojErrNone;
    }

    err = ActivityManager::getInstance().startActivity(act, msg);
    if (err) {
        LOG_AM_DEBUG("Start: Failed to start [Activity %llu]", act->getId());
    } else {
        LOG_AM_DEBUG("Start: [Activity %llu] started", act->getId());
    }

    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_stop stop

\e Public.

com.palm.activitymanager/stop

Stops an Activity and sends a "stop" event to all Activity subscribers. This
succeeds unless the Activity is already cancelled.

This is different from the cancel method in that more time is allowed for the
Activity to clean up. For example, this might matter for an email sync service
that needs more time to finish downloading a large email attachment. On a
"cancel", it should immediately abort the download, clean up, and exit. On a
"stop," it should finish downloading the attachment in some reasonable amount of
time--say, 10-15 seconds.

\subsection com_palm_activitymanager_stop_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.

\subsection com_palm_activitymanager_stop_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_stop_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/stop '{ "activityId": 876 }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "activityId not found",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::stopActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Stop: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);
    if (!validateCaller(Subscription::getBusId(msg), act)) {
        err = msg->replyError(MojErrAccessDenied, "caller is not a parent or creator");
        MojErrCheck(err);
        return MojErrNone;
    }

    act->plugAllSubscriptions();

    err = ActivityManager::getInstance().stopActivity(act);
    if (err) {
        LOG_AM_DEBUG("Stop: Failed to stop [Activity %llu]", act->getId());
        act->unplugAllSubscriptions();
    } else {
        LOG_AM_DEBUG("Stop: [Activity %llu] stopping", act->getId());
    }

    MojErrCheck(err);

    ensureCompletion(msg,
                     payload,
                     AbstractPersistManager::DeleteCommandType,
                     &ActivityCategoryHandler::finishStopActivity,
                     act);

    act->unplugAllSubscriptions();

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

void ActivityCategoryHandler::finishStopActivity(MojRefCountedPtr<MojServiceMessage> msg,
                                                 const MojObject& payload,
                                                 std::shared_ptr<Activity> act,
                                                 bool succeeded)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Stop finishing: Message from %s: [Activity %llu]: %s",
                 Subscription::getSubscriberString(msg).c_str(), act->getId(),
                 (succeeded ? "succeeded" : "failed"));

    if (act->isNameRegistered()) {
        ActivityManager::getInstance().unregisterActivityName(act);
    }

    MojErr err = msg->replySuccess();
    if (err)
        LOG_AM_ERROR(MSGID_STOP_ACTIVITY_REQ_REPLY_FAIL, 0,
                     "Failed to generate reply to Stop request");

    LOG_AM_DEBUG("Stop: [Activity %llu] stopped", act->getId());

    ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_cancel cancel

\e Public.

com.palm.activitymanager/cancel

Terminates the specified Activity and sends a "cancel" event to all subscribers.
This call should succeed if the Activity exists and is not already exiting.

This is different from the stop method in that the Activity should take little
or no time to clean up. For example, this might matter for an email sync service
that needs more time to finish downloading a large email attachment. On a
"cancel", it should immediately abort the download, clean up, and exit. On a
"stop", it should finish downloading the attachment in some reasonable amount of
time, say, 10-15 seconds. Note, however, that specific time limits are not
currently enforced, though this could change.

\subsection com_palm_activitymanager_cancel_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string
}
\endcode

\param activityId Activity ID. Either this or "activityName" is required.
\param activityName Activity name. Either this or "activityId" is required.

\subsection com_palm_activitymanager_cancel_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_cancel_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/cancel '{ "activityId": 123 }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "Activity ID or name not present in request",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::cancelActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Cancel: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);
    if (!validateCaller(Subscription::getBusId(msg), act)) {
        err = msg->replyError(MojErrAccessDenied, "caller is not a parent or creator");
        MojErrCheck(err);
        return MojErrNone;
    }

    act->plugAllSubscriptions();

    err = ActivityManager::getInstance().cancelActivity(act);
    if (err) {
        LOG_AM_DEBUG("Cancel: Failed to cancel [Activity %llu]", act->getId());
        act->unplugAllSubscriptions();
    } else {
        LOG_AM_DEBUG("Cancel: [Activity %llu] cancelling", act->getId());
    }

    MojErrCheck(err);

    ensureCompletion(msg, payload, AbstractPersistManager::DeleteCommandType,
                     &ActivityCategoryHandler::finishCancelActivity, act);

    act->unplugAllSubscriptions();

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

void ActivityCategoryHandler::finishCancelActivity(MojRefCountedPtr<MojServiceMessage> msg,
                                                   const MojObject& payload,
                                                   std::shared_ptr<Activity> act,
                                                   bool succeeded)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Cancel finishing: Message from %s: [Activity %llu] : %s",
                 Subscription::getSubscriberString(msg).c_str(), act->getId(),
                 succeeded ? "succeeded" : "failed");

    if (act->isNameRegistered()) {
        ActivityManager::getInstance().unregisterActivityName(act);
    }

    MojErr err = msg->replySuccess();
    if (err)
        LOG_AM_ERROR(MSGID_CANCEL_ACTIVITY_REQ_REPLY_FAIL, 0,
                     "Failed to generate reply to Cancel request");

    LOG_AM_DEBUG("Cancel: [Activity %llu] cancelled", act->getId());

    ACTIVITY_SERVICEMETHODFINISH_END(msg);
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_pause pause

\e Public.

com.palm.activitymanager/pause

Requests that the specified Activity be placed into the "paused" state. This
should succeed if the Activity is currently running or paused, but fail if the
Activity has ended (and is waiting for its subscribers to unsubscribe before
cleaning up) or has been cancelled.

\subsection com_palm_activitymanager_pause_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.

\subsection com_palm_activitymanager_pause_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_pause_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/pause '{ "activityId": 81 }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "Error retrieving activityId of Activity to operate on",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::pauseActivity(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Pause: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);
    if (!validateCaller(Subscription::getBusId(msg), act)) {
        err = msg->replyError(MojErrAccessDenied, "caller is not a parent or creator");
        MojErrCheck(err);
        return MojErrNone;
    }

    err = ActivityManager::getInstance().pauseActivity(act);
    if (err) {
        LOG_AM_DEBUG("Pause: Failed to pause [Activity %llu]", act->getId());
    } else {
        LOG_AM_DEBUG("Pause: [Activity %llu] pausing", act->getId());
    }

    MojErrCheck(err);

    err = msg->replySuccess();
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_list list

\e Public.

com.palm.activitymanager/list

List activities that are in running or waiting state.

\subsection com_palm_activitymanager_list_syntax Syntax:
\code
{
    "details": boolean,
    "subscribers": boolean,
    "current": boolean,
    "internal": boolean
}
\endcode

\param details Set to true to return full details for each Activity.
\param subscribers Set to true to return current parent, queued adopters, and
                   subscribers for each Activity
\param current Set to true to return the *current* state of the prerequisites
               for the Activity, as opposed to the desired states
\param internal Set to true to Include internal state information for debugging.


\subsection com_palm_activitymanager_list_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "activities": [ object array ],
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param activities Array with Activity objects.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_list_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.activitymanager/list '{ "details": true }'
\endcode

Example response for a succesful call:
\code
{
    "activities": [
        {
            "activityId": 2,
            "callback": {
                "method": "palm:\/\/com.palm.service.backup\/registerPubSub",
                "params": {
                }
            },
            "creator": {
                "serviceId": "com.palm.service.backup"
            },
            "description": "Registers the Backup Service with the PubSub Service, once the PubSub Service has a session.",
            "focused": false,
            "name": "RegisterPubSub",
            "state": "waiting",
            "trigger": {
                "method": "palm:\/\/com.palm.pubsubservice\/subscribeConnStatus",
                "params": {
                    "subscribe": true
                },
                "where": [
                    {
                        "op": "=",
                        "prop": "session",
                        "val": true
                    }
                ]
            },
            "type": {
                "background": true,
                "bus": "private"
            }
        },
        ...
        {
            "activityId": 223,
            "callback": {
                "method": "palm:\/\/com.palm.smtp\/syncOutbox",
                "params": {
                    "accountId": "++IAHN1DN8Snvorv",
                    "folderId": "++IAHN1nZl4k1OtL"
                }
            },
            "creator": {
                "serviceId": "com.palm.smtp"
            },
            "description": "Watches SMTP outbox for new emails",
            "focused": false,
            "metadata": {
                "accountId": "++IAHN1DN8Snvorv",
                "folderId": "++IAHN1nZl4k1OtL"
            },
            "name": "SMTP Watch\/accountId=\"\"++IAHN1DN8Snvorv\"\"",
            "requirements": {
                "internet": true
            },
            "state": "waiting",
            "trigger": {
                "key": "fired",
                "method": "palm:\/\/com.palm.db\/watch",
                "params": {
                    "query": {
                        "from": "com.palm.email:1",
                        "where": [
                            {
                                "_id": "454",
                                "op": "=",
                                "prop": "folderId",
                                "val": "++IAHN1nZl4k1OtL"
                            }
                        ]
                    }
                }
            },
            "type": {
                "bus": "private",
                "explicit": true,
                "immediate": true,
                "persist": true,
                "priority": "low"
            }
        }
    ],
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": -1989,
    "errorText": "json: unexpected char at 1:14",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::listActivities(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("List: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;
    if (!payload.contains(_T("details"))) {
        err = msg->replyError(MojErrInvalidArg, "'details flag' must be specified");
        MojErrCheck(err);
        return MojErrNone;
    }
    if (!payload.contains(_T("subscribers"))) {
        err = msg->replyError(MojErrInvalidArg, "'subscribers flag' must be specified");
        MojErrCheck(err);
        return MojErrNone;
    }
    if (!payload.contains(_T("current"))) {
        err = msg->replyError(MojErrInvalidArg, "'current flag' must be specified");
        MojErrCheck(err);
        return MojErrNone;
    }
    if (!payload.contains(_T("internal"))) {
        err = msg->replyError(MojErrInvalidArg, "'internal flag' must be specified");
        MojErrCheck(err);
        return MojErrNone;
    }

    bool found = false;
    if (payload.contains(_T("activityId"))) {
        err = payload.del(_T("activityId"), found);
        MojErrCheck(err);
    }
    if (payload.contains(_T("activityName"))) {
        err = payload.del(_T("activityName"), found);
        MojErrCheck(err);
    }
    err = getActivityInfo(msg, payload);
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_get_details getDetails

\e Public.

com.palm.activitymanager/getDetails

Get details of an activity.

\subsection com_palm_activitymanager_get_details_syntax Syntax:
\code
{
    "activityId": int,
    "activityName": string,
    "current": boolean,
    "internal": boolean
}
\endcode

\param activityId Activity ID. Either this, or "activityName" is required.
\param activityName Activity name. Either this, or "activityId" is required.
\param current Set to true to return the *current* state of the prerequisites
               for the Activity, as opposed to the desired states
\param internal Set to true to Include internal state information for debugging.

\subsection com_palm_activitymanager_get_details_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "activity": { object },
    "returnValue": boolean
}
\endcode

\param errorCode Code for the error in case the call was not succesful.
\param errorText Describes the error if the call was not succesful.
\param activity The activity object.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_activitymanager_get_details_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/getDetails '{ "activityId": 221, "current": true }'
\endcode

Example response for a succesful call:
\code
{
    "activity": {
        "activityId": 221,
        "adopters": [
        ],
        "callback": <NULL>,
        "creator": {
            "serviceId": "com.palm.smtp"
        },
        "description": "Watches SMTP config on account \"++IAHN1DN8Snvorv\"",
        "focused": false,
        "metadata": {
            "accountId": "++IAHN1DN8Snvorv"
        },
        "name": "SMTP Account Watch\/accountId=\"\"++IAHN1DN8Snvorv\"\"",
        "state": "waiting",
        "subscribers": [
        ],
        "trigger": false,
        "type": {
            "bus": "private",
            "explicit": true,
            "immediate": true,
            "persist": true,
            "priority": "low"
        }
    },
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": 2,
    "errorText": "Activity name\/creator pair not found",
    "returnValue": false
}
\endcode
*/

MojErr ActivityCategoryHandler::getActivityDetails(MojServiceMessage *msg,
                                                   MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Details: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    MojErr err = MojErrNone;

    std::shared_ptr<Activity> act;

    err = lookupActivity(msg, payload, act);
    MojErrCheck(err);

    if (!payload.contains(_T("current"))) {
        err = msg->replyError(MojErrInvalidArg, "'current flag' must be specified");
        MojErrCheck(err);
        return MojErrNone;
    }
    if (!payload.contains(_T("internal"))) {
        err = msg->replyError(MojErrInvalidArg, "'internal flag' must be specified");
        MojErrCheck(err);
        return MojErrNone;
    }

    err = payload.putBool(_T("subscribers"), true);
    MojErrCheck(err);
    err = payload.putBool(_T("details"), true);
    MojErrCheck(err);

    err = getActivityInfo(msg, payload);
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

/*!
\page com_palm_activitymanager
\n
\section com_palm_activitymanager_info info

\e Public.

com.palm.activitymanager/info

Get activity manager related information:
\li Activity Manager state:  Run queues and leaked Activities.
\li List of Activities for which power is currently locked.
\li State of the Resource Manager(s).

\subsection com_palm_activitymanager_info_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_activitymanager_info_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/info '{ }'
\endcode

Example response for a succesful call:
\code
{
    "queues": [
        {
            "activities": [
                {
                    "activityId": 28,
                    "creator": {
                        "serviceId": "com.palm.service.contacts.linker"
                    },
                    "name": "linkerWatch"
                },
                ...
                {
                    "activityId": 10,
                    "creator": {
                        "serviceId": "com.palm.db"
                    },
                    "name": "mojodbspace"
                }
            ],
            "name": "scheduled"
        }
    ],
    "resourceManager": {
        "entities": [
            {
                "activities": [
                ],
                "appId": "1",
                "focused": false,
                "priority": "none"
            },
            ...
            {
                "activities": [
                ],
                "anonId": "3",
                "focused": false,
                "priority": "none"
            }
        ],
        "resources": {
            "cpu": {
                "containers": [
                    {
                        "entities": [
                            {
                                "focused": false,
                                "priority": "none",
                                "serviceId": "com.palm.accountservices"
                            }
                        ],
                        "focused": false,
                        "name": "com.palm.accountservices",
                        "path": "\/dev\/cgroups\/com.palm.accountservices",
                        "priority": "low"
                    },
                    ...
                    {
                        "entities": [
                            {
                                "focused": false,
                                "priority": "none",
                                "serviceId": "2"
                            },
                            {
                                "anonId": "3",
                                "focused": false,
                                "priority": "none"
                            },
                            {
                                "appId": "1",
                                "focused": false,
                                "priority": "none"
                            }
                        ],
                        "focused": false,
                        "name": "container",
                        "path": "\/dev\/cgroups\/container",
                        "priority": "low"
                    }
                ],
                "entityMap": [
                    {
                        "serviceId:com.palm.netroute": "com.palm.netroute"
                    },
                    {
                        "serviceId:com.webos.service.connectionmanager": "com.palm.netroute"
                    },
                    ...
                    {
                        "serviceId:com.palm.preferences": "com.palm.preferences"
                    }
                ]
            }
        }
    }
}
\endcode
*/

MojErr ActivityCategoryHandler::getManagerInfo(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;

    MojObject reply(MojObject::TypeObject);

    /* Get the Activity Manager state:  Run queues and leaked Activities */
    err = ActivityManager::getInstance().infoToJson(reply);
    MojErrCheck(err);

    /* Get the list of Activities for which power is currently locked */
    err = PowerManager::getInstance().infoToJson(reply);
    MojErrCheck(err);

    err = RequirementManager::getInstance().infoToJson(reply);
    MojErrCheck(err);

    err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
    MojErrCheck(err);

    err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
    MojErrCheck(err);

    err = msg->reply(reply);
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

MojErr ActivityCategoryHandler::getActivityInfo(MojServiceMessage *msg, MojObject& payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Details: Message from %s: %s",
                 Subscription::getSubscriberString(msg).c_str(),
                 MojoObjectJson(payload).c_str());

    bool subscribers = true;
    bool details = true;
    bool current = false;
    bool internal = false;

    unsigned flags = 0;
    MojErr err = MojErrNone;

    payload.get(_T("subscribers"), subscribers);
    if (subscribers) {
        flags |= ACTIVITY_JSON_SUBSCRIBERS;
    }
    payload.get(_T("details"), details);
    if (details) {
        flags |= ACTIVITY_JSON_DETAIL;
    }
    payload.get(_T("current"), current);
    if (current) {
        flags |= ACTIVITY_JSON_CURRENT;
    }
    payload.get(_T("internal"), internal);
    if (internal) {
        flags |= ACTIVITY_JSON_INTERNAL;
    }

    MojObject reply;
    std::shared_ptr<Activity> act;
    if (payload.contains(_T("activityId")) || payload.contains(_T("activityName"))) {
        err = lookupActivity(msg, payload, act);
        MojErrCheck(err);

        MojObject activity;
        err = act->toJson(activity, flags);
        MojErrCheck(err);

        err = reply.put(_T("activity"), activity);
        MojErrCheck(err);
    } else {
        const ActivityManager::ActivityVec activities = ActivityManager::getInstance().getActivities();

        MojObject activityArray(MojObject::TypeArray);
        for (ActivityManager::ActivityVec::const_iterator iter = activities.begin();
                iter != activities.end(); ++iter) {
            MojObject activity;
            err = (*iter)->toJson(activity, flags);
            MojErrCheck(err);

            err = activityArray.push(activity);
            MojErrCheck(err);
        }

        err = reply.put(_T("activities"), activityArray);
        MojErrCheck(err);
    }

    err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
    MojErrCheck(err);

    err = msg->reply(reply);
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

MojErr ActivityCategoryHandler::replyDeprecatedMethod(MojServiceMessage *msg, MojObject& payload) {
    ACTIVITY_SERVICEMETHOD_BEGIN();

    MojErr err;
    err = msg->replyError(MojErrInternal, "Do not use this method because it is invalid");
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}

MojErr ActivityCategoryHandler::lookupActivity(MojServiceMessage *msg,
                                               MojObject& payload,
                                               std::shared_ptr<Activity>& act)
{
    try {
        act = ActivityExtractor::lookupActivity(payload, Subscription::getBusId(msg));
    } catch (const std::exception& except) {
        MojErr err = msg->replyError(MojErrNotFound, except.what());
        MojErrCheck(err);
        return MojErrNotFound;
    }

    return MojErrNone;
}

bool ActivityCategoryHandler::validateCaller(BusId caller, std::shared_ptr<Activity>& act)
{
    if (!Config::getInstance().validateCallerEnabled()) {
        return true;
    }

    if (caller.isLunaSend()) {
        return true;
    }

    if (act->hasParent()) {
        return act->getParent() == caller;
    }

    return act->getCreator() == caller;
}

bool ActivityCategoryHandler::subscribeActivity(MojServiceMessage *msg,
                                                const MojObject& payload,
                                                const std::shared_ptr<Activity>& act,
                                                std::shared_ptr<Subscription>& sub)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    bool subscribe = false;
    payload.get(_T("subscribe"), subscribe);

    if (!subscribe)
        return false;

    bool detailedEvents = false;
    payload.get(_T("detailedEvents"), detailedEvents);

    sub = std::make_shared<Subscription>(act, detailedEvents, msg);
    sub->enableSubscription();
    act->addSubscription(sub);

    return true;
}

MojErr ActivityCategoryHandler::checkSerial(MojServiceMessage *msg,
                                            const MojObject& payload,
                                            const std::shared_ptr<Activity>& act)
{
    MojErr err;
    bool found = false;

    MojUInt32 serial;

    err = payload.get(_T("serial"), serial, found);
    MojErrCheck(err);

    if (found) {
        if (!act->hasCallback()) {
            throw std::runtime_error(
                    "Attempt to use callback sequence serial "
                    "number match on Activity that does not have a callback");
        }

        if (act->getCallback()->getSerial() != serial) {
            LOG_AM_ERROR(
                    MSGID_SERIAL_NUM_MISMATCH,
                    4,
                    PMLOGKFV("Activity", "%llu", act->getId()),
                    PMLOGKS("subscriber_string", Subscription::getSubscriberString(msg).c_str()),
                    PMLOGKFV("required_serial", "%u",serial),
                    PMLOGKFV("current_serial", "%u", act->getCallback()->getSerial()),
                    "");
            throw std::runtime_error("Call sequence serial numbers do not match");
        }
    }

    return MojErrNone;
}

void ActivityCategoryHandler::ensureCompletion(MojServiceMessage *msg,
                                               MojObject& payload,
                                               AbstractPersistManager::CommandType type,
                                               CompletionFunction func,
                                               std::shared_ptr<Activity> act)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (act->isPersistent()) {
        /* Ensure a Persist Token object has been allocated for the Activity.
         * This should only be necessary on initial Create of the Activity. */
        if (!act->isPersistTokenSet()) {
            act->setPersistToken(DB8Manager::getInstance().createToken());
        }

        std::shared_ptr<ICompletion> completion =
                std::make_shared<MojoMsgCompletion<ActivityCategoryHandler>>(this, func, msg, payload, act);

        std::shared_ptr<AbstractPersistCommand> cmd =
            DB8Manager::getInstance().prepareCommand(type, act, completion);

        if (act->isPersistCommandHooked()) {
            act->hookPersistCommand(cmd);
            act->getHookedPersistCommand()->append(cmd);
        } else {
            act->hookPersistCommand(cmd);
            cmd->persist();
        }
    } else if (act->isPersistCommandHooked()) {
        std::shared_ptr<ICompletion> completion =
                std::make_shared<MojoMsgCompletion<ActivityCategoryHandler>>(this, func, msg, payload, act);

        std::shared_ptr<AbstractPersistCommand> cmd =
                DB8Manager::getInstance().prepareNoopCommand(act, completion);

        act->hookPersistCommand(cmd);
        act->getHookedPersistCommand()->append(cmd);
    } else {
        ((*this).*func)(msg, payload, act, true);
    }
}

/* XXX TEST ALL THE CASES HERE */
void ActivityCategoryHandler::ensureReplaceCompletion(
        MojServiceMessage *msg, MojObject& payload, std::shared_ptr<Activity> oldActivity,
        std::shared_ptr<Activity> newActivity)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (!oldActivity) {
        LOG_AM_ERROR(MSGID_NO_OLD_ACTIVITY, 1,
                     PMLOGKFV("new_activity","%llu", newActivity->getId()),
                     "Activity which new Activity is to replace was not specified");
        throw std::runtime_error("Activity to be replaced must be specified");
    }

    if (newActivity->isPersistTokenSet()) {
        LOG_AM_ERROR(
                MSGID_NEW_ACTIVITY_PERSIST_TOKEN_SET,
                2,
                PMLOGKFV("new_activity", "%llu", newActivity->getId()),
                PMLOGKFV("old_activity", "%llu", oldActivity->getId()),
                "New activity already has persist token set while preparing it to replace old activity");
        throw std::runtime_error("New Activity already has persist token "
                                 "set while replacing existing Activity");
    }

    if (newActivity->isPersistent()) {
        /* Activities can only get Tokens on Create (or Load at startup).  If
         * it's missing it's because the Activity was never persistent or was
         * cancelled */
        if (oldActivity->isPersistTokenSet()) {
            newActivity->setPersistToken(oldActivity->getPersistToken());
        } else {
            newActivity->setPersistToken(DB8Manager::getInstance().createToken());
        }

        std::shared_ptr<ICompletion> newCompletion =
                std::make_shared<MojoMsgCompletion<ActivityCategoryHandler>>(
                        this,
                        &ActivityCategoryHandler::finishCreateActivity,
                        msg,
                        payload,
                        newActivity);

        std::shared_ptr<AbstractPersistCommand> newCmd =
                DB8Manager::getInstance().prepareStoreCommand(newActivity, newCompletion);

        std::shared_ptr<ICompletion> oldCompletion =
                std::make_shared<MojoRefCompletion<ActivityCategoryHandler>>(
                        this,
                        &ActivityCategoryHandler::finishReplaceActivity,
                        oldActivity);

        std::shared_ptr<AbstractPersistCommand> oldCmd =
                DB8Manager::getInstance().prepareNoopCommand(oldActivity, oldCompletion);

        /* New command first (it's the blocking one), then old command */
        newActivity->hookPersistCommand(newCmd);
        newCmd->append(oldCmd);

        if (oldActivity->isPersistCommandHooked()) {
            oldActivity->hookPersistCommand(oldCmd);
            oldActivity->getHookedPersistCommand()->append(newCmd);
        } else {
            oldActivity->hookPersistCommand(oldCmd);
            newCmd->persist();
        }
    } else if (oldActivity->isPersistent()) {
        /* Prepare command to delete the old Activity */
        std::shared_ptr<ICompletion> oldCompletion =
                std::make_shared<MojoRefCompletion<ActivityCategoryHandler>>(
                        this,
                        &ActivityCategoryHandler::finishReplaceActivity,
                        oldActivity);

        std::shared_ptr<AbstractPersistCommand> oldCmd =
                DB8Manager::getInstance().prepareDeleteCommand(oldActivity, oldCompletion);

        /* Ensure there's a command attached to the new Activity in case
         * someone tries to replace *it*.  That can't succeed until the
         * original command is succesfully replaced.  This noop command
         * will complete the replace. */
        std::shared_ptr<ICompletion> newCompletion =
                std::make_shared<MojoMsgCompletion<ActivityCategoryHandler>>(
                        this,
                        &ActivityCategoryHandler::finishCreateActivity,
                        msg,
                        payload,
                        newActivity);

        std::shared_ptr<AbstractPersistCommand> newCmd =
                DB8Manager::getInstance().prepareNoopCommand(newActivity, newCompletion);

        /* Old command first (it's the blocking one), then complete the
         * new command - the create. */
        newActivity->hookPersistCommand(newCmd);
        oldCmd->append(newCmd);

        /* Remember to hook the command *after* checking if there is already
         * one hooked. */
        if (oldActivity->isPersistCommandHooked()) {
            oldActivity->hookPersistCommand(oldCmd);
            oldActivity->getHookedPersistCommand()->append(oldCmd);
        } else {
            oldActivity->hookPersistCommand(oldCmd);
            oldCmd->persist();
        }
    } else if (oldActivity->isPersistCommandHooked()) {
        /* Activity B (not persistent) replaced Activity A (persistent),
         * which is still being deleted. Now Activity C is trying to replace
         * Activity B, but since A hasn't finished yet, C shouldn't finish
         * either.  A crash might leave A existing while C existed
         * externally.  Which would be naughty. */
        std::shared_ptr<ICompletion> newCompletion =
                std::make_shared<MojoMsgCompletion<ActivityCategoryHandler>>(
                        this,
                        &ActivityCategoryHandler::finishCreateActivity,
                        msg,
                        payload,
                        newActivity);

        std::shared_ptr<AbstractPersistCommand> newCmd =
                DB8Manager::getInstance().prepareNoopCommand(newActivity, newCompletion);

        std::shared_ptr<ICompletion> oldCompletion =
                std::make_shared<MojoRefCompletion<ActivityCategoryHandler>>(
                        this,
                        &ActivityCategoryHandler::finishReplaceActivity,
                        oldActivity);

        std::shared_ptr<AbstractPersistCommand> oldCmd =
                DB8Manager::getInstance().prepareNoopCommand(oldActivity, oldCompletion);

        /* Wait for whatever that command is ultimately waiting for */
        newActivity->hookPersistCommand(newCmd);
        newCmd->append(oldCmd);
        oldActivity->hookPersistCommand(oldCmd);
        oldActivity->getHookedPersistCommand()->append(newCmd);
    } else {
        finishCreateActivity(MojRefCountedPtr<MojServiceMessage>(msg),
                             payload,
                             newActivity,
                             true);
        finishReplaceActivity(oldActivity, true);
    }
}

