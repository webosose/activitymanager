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

#include "CompareMatcher.h"

CompareMatcher::CompareMatcher(const MojString& key, const MojObject& value)
        : m_key(key), m_value(value)
{
}

CompareMatcher::~CompareMatcher()
{
}

bool CompareMatcher::match(const MojObject& response)
{
    MojObject value;
    MojString valueString;
    MojString oldValueString;
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (!response.contains(m_key)) {
        LOG_AM_DEBUG("Compare Matcher: Comparison key (%s) not present.", m_key.data());
        return false;
    }

    response.get(m_key, value);
    value.stringValue(valueString);
    if (value == m_value) {
        LOG_AM_DEBUG("Compare Matcher: Comparison key \"%s\" value \"%s\" unchanged.",
                     m_key.data(), valueString.data());
        return false;
    }


    m_value.stringValue(oldValueString);
    LOG_AM_DEBUG(
            "Compare Matcher: Comparison key \"%s\" value changed from \"%s\" to \"%s\".  Firing.",
            m_key.data(), oldValueString.data(), valueString.data());

    return true;
}

MojErr CompareMatcher::toJson(MojObject& rep, unsigned long flags) const
{
    MojErr err;
    MojObject compare;

    err = compare.put(_T("key"), m_key);
    MojErrCheck(err);

    err = compare.put(_T("value"), m_value);
    MojErrCheck(err);

    err = rep.put(_T("compare"), compare);
    MojErrCheck(err);

    return MojErrNone;
}
