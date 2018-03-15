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
#include <stdexcept>
#include <cstdlib>

#include "activity/Activity.h"
#include "util/Logging.h"

MojLogger AbstractScheduleManager::s_log(_T("activitymanager.scheduler"));

AbstractScheduleManager::AbstractScheduleManager()
        : m_nextWakeup(0)
        , m_wakeScheduled(false)
        , m_localOffsetSet(false)
        , m_localOffset(0)
{
    /* Calculate a random base start time between 11pm and 5am so all
     * the devices don't cause a storm of syncs if their midnights are
     * aligned.  (Generally, local time should be used for that sort of thing,
     * so at least they'll be distributed globally.  Also, technically,
     * an hour spread would be ok, but there are longitudes with less
     * subscribers, so spreading the more populated neighbors farther will
     * still help.) */
    m_smartBase = (23 * 60 * 60) + (random() % (6 * 60 * 60));
}

AbstractScheduleManager::~AbstractScheduleManager()
{
}

void AbstractScheduleManager::addItem(std::shared_ptr<Schedule> item)
{
    bool updateWake = false;

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    LOG_AM_DEBUG(
            "Adding [Activity %llu] to start at %llu (%s)",
            item->getActivity()->getId(),
            (unsigned long long )item->getNextStartTime(),
            timeToString(item->getNextStartTime(), !item->isLocal()).c_str());

    if (item->isLocal()) {
        m_localQueue.insert(*item);

        if (m_localQueue.iterator_to(*item) == m_localQueue.begin()) {
            updateWake = true;
        }
    } else {
        m_queue.insert(*item);

        if (m_queue.iterator_to(*item) == m_queue.begin()) {
            updateWake = true;
        }
    }

    if (updateWake) {
        g_timeout_add(0, dequeueAndUpdateTimeout, this);
    }
}

void AbstractScheduleManager::removeItem(std::shared_ptr<Schedule> item)
{
    bool updateWake = false;

    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    LOG_AM_DEBUG("Removing [Activity %llu]", item->getActivity()->getId());

    try {
        /* Do NOT attempt to get an iterator to an item that isn't in a
         * container. */
        if (item->m_queueItem.is_linked()) {

            /* If the item is at the head of either queue, the time might
             * have changed.  Otherwise, it definitely didn't. */
            if (item->isLocal()) {
                if (m_localQueue.iterator_to(*item) == m_localQueue.begin()) {
                    updateWake = true;
                }
            } else {
                if (m_queue.iterator_to(*item) == m_queue.begin()) {
                    updateWake = true;
                }
            }

            item->m_queueItem.unlink();

            if (updateWake) {
                dequeueAndUpdateTimeout();
            }
        }
    } catch (...) {
    }
}

std::string AbstractScheduleManager::timeToString(time_t convert, bool isUTC)
{
    char buf[32];
    struct tm tm;

    memset(&tm, 0, sizeof(struct tm));
    gmtime_r(&convert, &tm);

    size_t len = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    if (isUTC) {
        buf[len] = 'Z';
        buf[len + 1] = '\0';
    }

    return std::string(buf);
}

time_t AbstractScheduleManager::stringToTime(const char *convert, bool& isUTC)
{
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    char *next = strptime(convert, "%Y-%m-%d %H:%M:%S", &tm);
    if (!next) {
        throw std::runtime_error("Failed to parse start time");
    }

    if (*next == 'Z') {
        isUTC = true;
    } else if (*next == '\0') {
        isUTC = false;
    } else {
        throw std::runtime_error("Start time must end in 'Z' for UTC, or nothing");
    }

    /* mktime performs conversions assuming the time is in the local timezone.
     * timezone must be set for UTC for this to behave properly. */
    return mktime(&tm);
}

void AbstractScheduleManager::setLocalOffset(off_t offset)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Setting local offset to %lld", (long long )offset);

    bool updateWake = false;

    if (!m_localOffsetSet) {
        m_localOffset = offset;
        m_localOffsetSet = true;
        updateWake = true;
    } else if (m_localOffset != offset) {
        m_localOffset = offset;
        updateWake = true;
    }

    if (updateWake) {
        dequeueAndUpdateTimeout();
    }
}

off_t AbstractScheduleManager::getLocalOffset() const
{
    if (!m_localOffsetSet) {
        LOG_AM_WARNING(MSGID_SCHE_OFFSET_NOTSET, 0,
                       "Attempt to access local offset before it has been set");
    }

    return m_localOffset;
}

time_t AbstractScheduleManager::getSmartBaseTime() const
{
    return m_smartBase;
}

void AbstractScheduleManager::wake()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Wake callback");

    m_wakeScheduled = false;

    dequeueAndUpdateTimeout();
}

gboolean AbstractScheduleManager::dequeueAndUpdateTimeout(gpointer data)
{
    AbstractScheduleManager* self = static_cast<AbstractScheduleManager*>(data);
    if (!self) {
        return G_SOURCE_REMOVE;
    }

    self->dequeueAndUpdateTimeout();
    return G_SOURCE_REMOVE;
}

/* XXX Handle timer rollover? */
void AbstractScheduleManager::dequeueAndUpdateTimeout()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    /* Nothing to do?  Then return.  A new timeout will be scheduled
     * the next time something is queued */
    if (m_queue.empty() && (!m_localOffsetSet || m_localQueue.empty())) {
        LOG_AM_DEBUG("Not dequeuing any items as queue is now empty");
        if (m_wakeScheduled) {
            cancelTimeout();
            m_wakeScheduled = false;
        }
        return;
    }

    time_t curTime = time(NULL);

    LOG_AM_DEBUG("Beginning to dequeue items at time %llu",(unsigned long long) curTime);

    /* If anything on the queue already happened in the past, dequeue it
     * and mark it as Scheduled(). */

    processQueue(m_queue, curTime);

    /* Only process the local queue if the timezone offset is known.
     * Otherwise, wait, because it will be known shortly. */
    if (m_localOffsetSet) {
        processQueue(m_localQueue, curTime + m_localOffset);
    }

    LOG_AM_DEBUG("Done dequeuing items");

    /* Both queues scheduled and dequeued (or unknown if time zone is not
     * yet known)? */
    if (m_queue.empty() && (!m_localOffsetSet || m_localQueue.empty())) {
        LOG_AM_DEBUG("No unscheduled items remain");

        if (m_wakeScheduled) {
            cancelTimeout();
            m_wakeScheduled = false;
        }

        return;
    }

    time_t nextWakeup = getNextStartTime();

    if (!m_wakeScheduled || (nextWakeup != m_nextWakeup)) {
        updateTimeout(nextWakeup, curTime);
        m_nextWakeup = nextWakeup;
        m_wakeScheduled = true;
    }
}

void AbstractScheduleManager::processQueue(ScheduleQueue& queue, time_t curTime)
{
    while (!queue.empty()) {
        Schedule& item = *(queue.begin());

        if (item.getNextStartTime() <= curTime) {
            item.m_queueItem.unlink();
            item.scheduled();
        } else {
            break;
        }
    }
}

void AbstractScheduleManager::reQueue(ScheduleQueue& queue)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("Requeuing");

    ScheduleQueue updated;

    while (!queue.empty()) {
        Schedule& item = *(queue.begin());

        item.m_queueItem.unlink();
        item.calcNextStartTime();
        updated.insert(item);
    }

    updated.swap(queue);
}

void AbstractScheduleManager::timeChanged()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG(
            "System time or timezone changed, recomputing start times and requeuing Scheduled Activities");

    reQueue(m_queue);
    reQueue(m_localQueue);

    dequeueAndUpdateTimeout();
}

time_t AbstractScheduleManager::getNextStartTime() const
{
    if (m_queue.empty()) {
        if (!m_localOffsetSet || m_localQueue.empty()) {
            throw std::runtime_error("No available items in queue");
        } else {
            const Schedule& localItem = *(m_localQueue.begin());
            return (localItem.getNextStartTime() - m_localOffset);
        }
    } else {
        if (!m_localOffsetSet || m_localQueue.empty()) {
            const Schedule& item = *(m_queue.begin());
            return item.getNextStartTime();
        } else {
            const Schedule& localItem = *(m_localQueue.begin());
            time_t nextLocalStartTime = (localItem.getNextStartTime() - m_localOffset);

            const Schedule& item = *(m_queue.begin());
            time_t nextStartTime = item.getNextStartTime();

            if (nextStartTime < nextLocalStartTime) {
                return nextStartTime;
            } else {
                return nextLocalStartTime;
            }
        }
    }
}

