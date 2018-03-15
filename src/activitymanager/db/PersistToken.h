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

#ifndef __PERSIST_TOKEN_H__
#define __PERSIST_TOKEN_H__

#include <string>

class PersistToken {
public:
    PersistToken();
    virtual ~PersistToken();

    virtual bool isValid() const = 0;

    virtual std::string getString() const = 0;
};

#endif /* __PERSIST_TOKEN_H__ */
