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

#ifndef __BUS_ID_H__
#define __BUS_ID_H__

#include <string>

#include "Main.h"

enum BusIdType {
    BusNull,
    BusApp,
    BusService,
    BusAnon
};

class Subscriber;

class BusId {
public:
    BusId();
    BusId(const std::string& id, BusIdType type);
    BusId(const char *id, BusIdType type);
    BusId(const Subscriber& subscriber);

    BusIdType getType() const;
    std::string getId() const;

    std::string getString() const;
    static std::string getString(const std::string& id, BusIdType type);
    static std::string getString(const char *id, BusIdType type);

    MojErr toJson(MojObject& rep) const;

    BusId& operator=(const Subscriber& rhs);

    bool operator<(const BusId& rhs) const;
    bool operator!=(const BusId& rhs) const;
    bool operator==(const BusId& rhs) const;

    bool operator!=(const Subscriber& rhs) const;
    bool operator==(const Subscriber& rhs) const;

    bool operator!=(const std::string& rhs) const;
    bool operator==(const std::string& rhs) const;

    bool isLunaSend() const;

protected:
    std::string m_id;
    BusIdType m_type;
};

#endif /* __BUS_ID_H__ */
