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

#ifndef __MATCHER_H__
#define __MATCHER_H__

#include "Main.h"

class Matcher {
public:
    Matcher();
    virtual ~Matcher();

    virtual bool match(const MojObject& response) = 0;
    virtual void reset();

    virtual MojErr toJson(MojObject& rep, unsigned long flags) const = 0;

protected:
    static MojLogger s_log;
};

#endif /* __MATCHER_H__ */
