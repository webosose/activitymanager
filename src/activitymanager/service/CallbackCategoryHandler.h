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

#ifndef __CALLBACK_CATEGORY_HANDLER_H__
#define __CALLBACK_CATEGORY_HANDLER_H__

#include <core/MojService.h>
#include <core/MojServiceMessage.h>

#include "Main.h"

class AbstractScheduleManager;

class CallbackCategoryHandler: public MojService::CategoryHandler {
public:
    CallbackCategoryHandler();
    virtual ~CallbackCategoryHandler();

    MojErr init();

protected:
    /* Wake Callback */
    MojErr scheduledWakeup(MojServiceMessage *msg, MojObject &payload);

    static MojLogger s_log;

    static const Method s_methods[];

};

#endif /* __CALLBACK_CATEGORY_HANDLER_H__ */
