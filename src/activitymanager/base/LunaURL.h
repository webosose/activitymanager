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

#ifndef __LUNA_URL_H__
#define __LUNA_URL_H__

#include <core/MojString.h>
#include <string>

class LunaURL {
public:
    LunaURL();
    LunaURL(const MojString& url);
    LunaURL(const char *data);
    LunaURL(const MojString& targetService, const MojString& method);

    virtual ~LunaURL();

    LunaURL& operator=(const char *url);
    LunaURL& operator=(const MojString& url);

    bool operator==(const LunaURL& other) const;
    bool operator!=(const LunaURL& other) const
    {
        return !operator ==(other);
    }

    MojString getTargetService() const;
    MojString getMethod() const;

    std::string getString() const;

protected:
    void init(const MojString& url);

    MojString m_targetService;
    MojString m_method;

    bool m_valid;
};

#endif /* __LUNA_URL_H__ */
