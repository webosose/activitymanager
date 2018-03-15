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

#include "KeyMatcher.h"

KeyMatcher::KeyMatcher(const MojString& key)
        : m_key(key)
{
}

KeyMatcher::~KeyMatcher()
{
}

bool KeyMatcher::match(const MojObject& response)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    /* If those were the droids we were looking for, fire! */
    if (response.contains(m_key)) {
        LOG_AM_DEBUG("Key Matcher: Key \"%s\" found in response %s",
                     m_key.data(), MojoObjectJson(response).c_str());
        return true;
    } else {
        LOG_AM_DEBUG("Key Matcher: Key \"%s\" not found in response %s",
                     m_key.data(), MojoObjectJson(response).c_str());
        return false;
    }
}

MojErr KeyMatcher::toJson(MojObject& rep, unsigned long flags) const
{
    MojErr err;

    err = rep.put(_T("key"), m_key);
    MojErrCheck(err);

    return MojErrNone;
}
