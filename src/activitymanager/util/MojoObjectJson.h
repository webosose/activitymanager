// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef __MOJO_OBJECT_JSON_H__
#define __MOJO_OBJECT_JSON_H__

#include <string>

#include <pbnjson.hpp>
#include <core/MojObject.h>

using namespace pbnjson;

class MojoObjectJson {
public:
    static JValue convertMojObjToJValue(const MojObject& obj);
    static MojObject convertJValueToMojObj(JValue& obj);

    MojoObjectJson(const MojObject& obj);
    virtual ~MojoObjectJson();

    std::string str() const;
    const char *c_str() const;

protected:
    MojString m_str;
};

#endif /* __MOJO_OBJECT_JSON_H__ */
