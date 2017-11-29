// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef __CONTINUOUS_ACTIVITY_H__
#define __CONTINUOUS_ACTIVITY_H__

#include "Activity.h"

class ContinuousActivity : public Activity {
public:
    ContinuousActivity(activityId_t id);
    virtual ~ContinuousActivity();

    virtual void onSuccessTrigger(std::shared_ptr<ITrigger> trigger, bool statusChanged, bool valueChanged);
    virtual void onMetRequirement(std::string name);
    virtual void scheduled();

    virtual void callbackSucceeded(std::shared_ptr<AbstractCallback> callback);
    virtual void doCallback();

    virtual bool isRunnable() const;
    virtual bool isShouldWait() const;
    virtual void unscheduleActivity();

private:
    /* DISALLOW */
    ContinuousActivity();
    ContinuousActivity(const ContinuousActivity& copy);
    ContinuousActivity& operator=(const ContinuousActivity& copy);
    bool operator==(const ContinuousActivity& rhs) const;

    bool pushToWaitQueue();

    std::deque<MojObject> m_activitySnapshots;
    bool m_shouldWait;
};

#endif /* __CONTINUOUS_ACTIVITY_H__ */
