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

#ifndef __IFAKE_CONDITION_H__
#define __IFAKE_CONDITION_H__

class IFakeCondition {
public:
    IFakeCondition() {};
    virtual ~IFakeCondition() {};

    virtual void setSatisfied(bool satisfied) = 0;
    virtual void unsetSatisfied() = 0;
};

#endif /* __IFAKE_CONDITION_H__ */
