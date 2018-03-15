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

#ifndef _ACTIVITY_MANAGER_APP_H_
#define _ACTIVITY_MANAGER_APP_H_

#include <core/MojGmainReactor.h>
#include <core/MojReactorApp.h>
#include "Main.h"

#include "activity/ActivityExtractor.h"
#include "activity/ActivityManager.h"
#include "requirement/RequirementManager.h"
#include "service/ActivityCategoryHandler.h"
#include "service/CallbackCategoryHandler.h"
#include "db/DB8Manager.h"

class ActivityManagerApp: public MojReactorApp<MojGmainReactor>, public DB8ManagerListener {
public:
    ActivityManagerApp();
    virtual ~ActivityManagerApp();

    virtual MojErr open();
    virtual MojErr ready();

    virtual void onFinish();
    virtual void onFail();

protected:
    MojErr online();
    void initRNG();

    char m_rngState[256];

private:
    typedef MojReactorApp<MojGmainReactor> Base;

    MojRefCountedPtr<ActivityCategoryHandler> m_handler;
    MojRefCountedPtr<CallbackCategoryHandler> m_callbackHandler;
    MojRefCountedPtr<DevelCategoryHandler> m_develHandler;

};

#endif /* _ACTIVITY_MANAGER_APP_H_ */
