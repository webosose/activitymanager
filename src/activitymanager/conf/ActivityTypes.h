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

#ifndef __ACTIVITY_TYPES_H__
#define __ACTIVITY_TYPES_H__

enum ActivityEvent {
    /* Commands */
    kActivityStartEvent,    /* start evnet is equals to call callback */
    kActivityStopEvent,
    kActivityPauseEvent,
    kActivityCancelEvent,
    kActivityCompleteEvent,
    kActivityYieldEvent,

    /* Events */
    kActivityOrphanEvent,   /* Only the parent who has just adopted an activity receives orphan event */
    kActivityAdoptedEvent,  /* Only released parent (but subscription is still kept) receives adopted event */
    kActivityUpdateEvent,
    MaxActivityEvent
};

extern const char *ActivityEventNames[];

typedef enum ActivityPriority_e {
    ActivityPriorityNone,
    ActivityPriorityLowest,
    ActivityPriorityLow,
    ActivityPriorityNormal,
    ActivityPriorityHigh,
    ActivityPriorityHighest,
    MaxActivityPriority
} ActivityPriority_t;

extern const char *ActivityPriorityNames[];

extern const ActivityPriority_t ActivityForegroundPriority;
extern const ActivityPriority_t ActivityBackgroundPriority;

#endif /* __ACTIVITY_TYPES_H__ */
