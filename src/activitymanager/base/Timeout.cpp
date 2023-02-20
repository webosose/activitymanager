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

#include "Timeout.h"

#include "util/Logging.h"

MojLogger TimeoutBase::s_log(_T("activitymanager.timeout"));

TimeoutBase::TimeoutBase(unsigned seconds)
    : m_seconds(seconds)
    , m_timeout(NULL)
{
}

TimeoutBase::~TimeoutBase()
{
    cancel();
}

void TimeoutBase::arm()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (m_timeout) {
        cancel();
    }

    GSource *timeout = g_timeout_source_new_seconds((guint)m_seconds);
    g_source_set_callback(timeout, TimeoutBase::staticWakeupTimeout, this, NULL);
    g_source_attach(timeout, g_main_context_default());


    m_timeout = timeout;
}

void TimeoutBase::cancel()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (!m_timeout) {
        return;
    }

    if (!g_source_is_destroyed(m_timeout)) {
        g_source_destroy(m_timeout);
    }

    m_timeout = NULL;
}

gboolean TimeoutBase::staticWakeupTimeout(gpointer data)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Wakeup");

    TimeoutBase *base = static_cast<TimeoutBase *>(data);

    /* Reset NOW, because the timeout call could delete this object. */
    base->cancel();

    try {
        base->wakeupTimeout();
    } catch (const std::exception& except) {
        LOG_AM_ERROR(MSGID_TIMEOUT_EXCEPTION,0, "Unhandled exception \"%s\" occurred",except.what());
    } catch (...) {
        LOG_AM_ERROR(MSGID_TIMEOUT_ERR_UNKNOWN,0, "Unhandled exception of unknown type occurred");
    }

    return false;
}

