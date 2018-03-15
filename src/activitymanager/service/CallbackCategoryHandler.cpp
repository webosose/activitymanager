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

#include <activity/schedule/AbstractScheduleManager.h>
#include <activity/schedule/ScheduleManager.h>
#include "CallbackCategoryHandler.h"

#include "Category.h"
#include "util/Logging.h"

// TODO: I could not get this method to do anything, so leaving out of the generated documentation
/* !
 * \page com_palm_activitymanager_callback Service API com.palm.activitymanager/callback/
 * Private methods:
 * - \ref com_palm_activitymanager_callback_scheduledwakeup
 */

const CallbackCategoryHandler::Method CallbackCategoryHandler::s_methods[] = {
        { _T("scheduledwakeup"), (Callback) &CallbackCategoryHandler::scheduledWakeup },
        { NULL, NULL } };

MojLogger CallbackCategoryHandler::s_log(_T("activitymanager.callbackhandler"));

CallbackCategoryHandler::CallbackCategoryHandler()
{
}

CallbackCategoryHandler::~CallbackCategoryHandler()
{
}

MojErr CallbackCategoryHandler::init()
{
    MojErr err;

    err = addMethods(s_methods);
    MojErrCheck(err);

    return MojErrNone;
}

/* !
\page com_palm_activitymanager_callback
\n
\section com_palm_activitymanager_callback_scheduledwakeup scheduledwakeup

\e Private.

com.palm.activitymanager/callback/scheduledwakeup



\subsection com_palm_activitymanager_callback_scheduledwakeup_syntax Syntax:
\code
{
}
\endcode

\param

\subsection com_palm_activitymanager_callback_scheduledwakeup_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string,
    "returnValue": boolean
}
\endcode

\param

\subsection com_palm_activitymanager_callback_scheduledwakeup_examples Examples:
\code
luna-send -i -f luna://com.palm.activitymanager/callback/scheduledwakeup '{ }'
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
    "errorCode": -1000,
    "errorText": "Internal exception of unknown type thrown",
    "returnValue": false
}
\endcode
*/

MojErr CallbackCategoryHandler::scheduledWakeup(MojServiceMessage *msg,
                                                MojObject &payload)
{
    ACTIVITY_SERVICEMETHOD_BEGIN();

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Callback: ScheduledWakeup");

    ScheduleManager::getInstance().scheduledWakeup();

    MojErr err = msg->replySuccess();
    MojErrCheck(err);

    ACTIVITY_SERVICEMETHOD_END(msg);

    return MojErrNone;
}
