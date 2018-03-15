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

#ifndef __PERSIST_TOKEN_DB_H__
#define __PERSIST_TOKEN_DB_H__

#include <core/MojCoreDefs.h>

#include "Main.h"
#include "PersistToken.h"

class PersistTokenDB : public PersistToken {
public:
    PersistTokenDB();
    PersistTokenDB(const MojString& id, MojInt64 rev);
    virtual ~PersistTokenDB();

    virtual bool isValid() const;

    void set(const MojString& id, MojInt64 rev);
    void update(const MojString& id, MojInt64 rev);
    void clear();

    MojInt64 getRev() const;
    const MojString& getId() const;

    MojErr toJson(MojObject& rep) const;

    std::string getString() const;

protected:
    bool m_valid;
    MojString m_id;
    MojInt64 m_rev;
};

#endif /* __PERSIST_TOKEN_DB_H__ */
