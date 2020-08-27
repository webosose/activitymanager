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

#include <activity/schedule/ScheduleManager.h>
#include <activity/type/PowerManager.h>
#include <db/DB8Manager.h>
#include "ActivityManagerApp.h"

#include <cstdlib>
#include <ctime>
#include <glib.h>
#ifdef INIT_MANAGER_systemd
#include <systemd/sd-daemon.h>
#endif

#include "conf/Config.h"
#include "tools/ActivityMonitor.h"
#include "tools/ActivitySendHandler.h"
#include "service/PermissionManager.h"
#include "util/Logging.h"
#include "service/BusConnection.h"
#include "conf/Setting.h"
#include <cstring>

ActivityManagerApp::ActivityManagerApp()
{
}

ActivityManagerApp::~ActivityManagerApp()
{
}

MojErr ActivityManagerApp::open()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("%s initializing", name().data());

    initRNG();

    MojErr err = Base::open();
    MojErrCheck(err);

    // Bring the Activity Manager client online
    err = BusConnection::getInstance().getClient().open(ActivityManager::kClientName);
    MojErrCheck(err);
    err = BusConnection::getInstance().getClient().attach(m_reactor.impl());
    MojErrCheck(err);

    try {
        DB8Manager::getInstance().addListener(this);
        RequirementManager::getInstance().initialize();
        ActivityMonitor::getInstance().initialize();
        ActivitySendHandler::getInstance().initialize();
    } catch (...) {
        return MojErrNoMem;
    }

    LOG_AM_DEBUG("%s initialized", name().data());

    /* System is initialized.  All object managers are prepared to instantiate
     * their objects as Activities are loaded from the database.  Begin
     * deserializing persistent Activities. */
    DB8Manager::getInstance().loadActivities();

    return MojErrNone;
}

MojErr ActivityManagerApp::ready()
{
    LOG_AM_DEBUG("%s ready to accept incoming requests", name().data());

    /* All stored Activities have been deserialized from the database.  All
     * previously persisted Activity IDs are marked as used.  It's safe to
     * accept requests.  Bring up the Service side interface! */
    MojErr err = online();
    MojErrCheck(err);

    /* Start up the various requirement management proxy connections now,
     * so they don't start a storm of requests once the UI is up. */
    RequirementManager::getInstance().enable();

    /* Subscribe to timezone notifications. */
    ScheduleManager::getInstance().enable();

#ifdef INIT_MANAGER_systemd
    if (sd_notify(0, "READY=1\nSTATUS=ActivityManager is ready\n") <= 0) {
#else
    if (::system("/sbin/initctl emit --no-wait activitymanager-ready") == -1) {
#endif
        LOG_AM_ERROR(MSGID_UPSTART_EMIT_FAIL, 0,
                     "ServiceApp: Failed to emit upstart event");
    }

    ActivityManager::getInstance().enable(ActivityManager::kServiceOnline);

    return MojErrNone;
}

void ActivityManagerApp::onFinish()
{
    ready();
}

void ActivityManagerApp::onFail()
{
    ready();
}

void ActivityManagerApp::initRNG()
{
    memset(m_rngState, 0, sizeof(m_rngState));

    unsigned int rngSeed;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    rngSeed = (unsigned) (tv.tv_sec ^ tv.tv_usec);

    LOG_AM_DEBUG("Seeding RNG using time: sec %u, usec %u, seed %u",
                 (unsigned )tv.tv_sec, (unsigned )tv.tv_usec, rngSeed);

    char *oldstate = initstate(rngSeed, m_rngState, sizeof(m_rngState));
    if (!oldstate) {
        LOG_AM_ERROR(MSGID_INIT_RNG_FAIL, 0, "Failed to initialize the RNG state");
    }
}

MojErr ActivityManagerApp::online()
{
    // Bring the Activity Manager online
    MojErr err = BusConnection::getInstance().getService().open(ActivityManager::kServiceName);
    MojErrCheck(err);
    err = BusConnection::getInstance().getService().attach(m_reactor.impl());
    MojErrCheck(err);

    /* Initialize main call handler:
     *    palm://com.palm.activitymanager/... */
    m_handler.reset(
            new ActivityCategoryHandler(std::make_shared<PermissionManager>()));
    MojAllocCheck(m_handler.get());

    err = m_handler->init();
    MojErrCheck(err);

    err = BusConnection::getInstance().getService().addCategory(MojLunaService::DefaultCategory, m_handler.get());
    MojErrCheck(err);

    /* Initialize callback category handler:
     *    palm://com.palm.activitymanager/callback/... */
    m_callbackHandler.reset(new CallbackCategoryHandler());
    MojAllocCheck(m_callbackHandler.get());

    err = m_callbackHandler->init();
    MojErrCheck(err);

    err = BusConnection::getInstance().getService().addCategory("/callback", m_callbackHandler.get());
    MojErrCheck(err);

    m_develHandler.reset(new DevelCategoryHandler());
    MojAllocCheck(m_develHandler.get());

    err = BusConnection::getInstance().getService().addCategory("/devel", m_develHandler.get());
    MojErrCheck(err);

    return MojErrNone;
}

