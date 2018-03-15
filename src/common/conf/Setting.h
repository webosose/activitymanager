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

#ifndef CONFIGURED_SRC_COMMON_CONF_SETTING_H_
#define CONFIGURED_SRC_COMMON_CONF_SETTING_H_

#include <iostream>
#include "conf/Environment.h"

using namespace std;

class Setting {
public:
    static Setting& getInstance()
    {
        static Setting _instance;
        return _instance;
    }
    virtual ~Setting() {};

    virtual bool isSimulator()
    {
        return m_isSimulator;
    }

private:
    Setting();

    bool m_isSimulator;

};

#endif /* CONFIGURED_SRC_COMMON_CONF_SETTING_H_ */
