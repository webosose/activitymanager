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

#include "MojoObjectJson.h"

JValue MojoObjectJson::convertMojObjToJValue(const MojObject& obj)
{
    MojString str;

    obj.toJson(str);
    return JDomParser::fromString(str.data());
}

MojObject MojoObjectJson::convertJValueToMojObj(JValue& obj)
{
    MojObject mojObj;
    MojString str;

    MojErr err = str.assign(obj.stringify("").c_str());
    if (MojErrNone != err) {
        return mojObj;
    }

    err = mojObj.fromJson(str);
    if (MojErrNone != err) {
        return mojObj;
    }

    return mojObj;
}

MojoObjectJson::MojoObjectJson(const MojObject& obj)
{
    MojErr err = obj.toJson(m_str);

    /* Intentially ignore err here.  This is for debug output, let's not
     * make the problem worse... */
    (void)err;
}

MojoObjectJson::~MojoObjectJson()
{
}

std::string MojoObjectJson::str() const
{
    return std::string(m_str.data());
}

const char *MojoObjectJson::c_str() const
{
    return m_str.data();
}

