// Copyright (c) 2017-2020 LG Electronics, Inc.
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

#ifndef __ACTIVITY_MONITOR_H__
#define __ACTIVITY_MONITOR_H__

#include <memory>

#include "activity/Activity.h"
#include "conf/ActivityTypes.h"

class ActivityMonitor {
public:
    static ActivityMonitor& getInstance();

    static gboolean onRead(GIOChannel* channel, GIOCondition condition, gpointer data);

    void initialize();

    void send(std::shared_ptr<Activity> activity);

private:
    ActivityMonitor();
    ActivityMonitor(const ActivityMonitor&) = delete;
    virtual ~ActivityMonitor();

    bool isMonitorRunning();
    void closeChannel();

    int m_pipeReq;
    int m_pipeResp;
    int m_isMonitorRunning;
    GIOChannel* m_channel;
    std::string m_ipcDir;
    std::string m_reqPipePath;
    std::string m_respPipePath;
};

#endif /* __ACTIVITY_MONITOR_H__ */
