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

#include "PersistTokenDB.h"

#include <sstream>
#include <stdexcept>
#include <core/MojObject.h>

PersistTokenDB::PersistTokenDB()
    : m_valid(false)
    , m_rev(-1)
{
}

PersistTokenDB::PersistTokenDB(const MojString& id, MojInt64 rev)
    : m_valid(true)
    , m_id(id)
    , m_rev(rev)
{
}

PersistTokenDB::~PersistTokenDB()
{
}

bool PersistTokenDB::isValid() const
{
    return m_valid;
}

void PersistTokenDB::set(const MojString& id, MojInt64 rev)
{
    if (m_valid) {
        throw std::runtime_error("ID already set");
    }

    m_valid = true;
    m_id = id;
    m_rev = rev;
}

void PersistTokenDB::update(const MojString& id, MojInt64 rev)
{
    if (!m_valid) {
        throw std::runtime_error("Attempt to update revision of token with invalid id");
    } else if (id != m_id) {
        throw std::runtime_error("Ids don't match attempting to update persist token");
    } else if (rev < m_rev) {
        throw std::runtime_error("New revision must be greater than or equal to current revision when updating");
    }

    m_rev = rev;
}

void PersistTokenDB::clear()
{
    m_valid = false;
    m_id.clear();
    m_rev = -1;
}

MojInt64 PersistTokenDB::getRev() const
{
    if (!m_valid) {
        throw std::runtime_error("Revision not set");
    }

    return m_rev;
}

const MojString& PersistTokenDB::getId() const
{
    if (!m_valid) {
        throw std::runtime_error("ID not set");
    }

    return m_id;
}

MojErr PersistTokenDB::toJson(MojObject& rep) const
{
    MojErr err;

    if (!m_valid) {
        throw std::runtime_error("ID not set");
    }

    err = rep.putString(_T("_id"), m_id);
    MojErrCheck(err);

    err = rep.putInt(_T("_rev"), m_rev);
    MojErrCheck(err);

    return MojErrNone;
}

std::string PersistTokenDB::getString() const
{
    if (!m_valid) {
        return "(invalid token)";
    }

    std::stringstream tokenStr;
    tokenStr << "(id: \"" << m_id.data() << "\", rev: " << m_rev << ")";
    return tokenStr.str();
}

