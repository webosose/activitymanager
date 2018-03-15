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

#ifndef __SUBSCRIBER_H__
#define __SUBSCRIBER_H__

#include <string>
#include <boost/intrusive/set.hpp>

#include "Main.h"
#include "BusId.h"

class Activity;

class Subscriber {
public:
    Subscriber(const std::string& id, BusIdType type);
    Subscriber(const char *id, BusIdType type);
    virtual ~Subscriber();

    BusIdType getType() const;
    const BusId& getId() const;

    std::string getString() const;

    MojErr toJson(MojObject& rep) const;

    bool operator<(const Subscriber& rhs) const;
    bool operator!=(const Subscriber& rhs) const;
    bool operator==(const Subscriber& rhs) const;

    bool operator!=(const BusId& rhs) const;
    bool operator==(const BusId& rhs) const;

    bool operator!=(const std::string& rhs) const;
    bool operator==(const std::string& rhs) const;

private:
    Subscriber(const Subscriber& subscriber);
    Subscriber& operator=(const Subscriber& subscriber);

protected:
    Subscriber();

    friend class Activity;

    typedef boost::intrusive::set_member_hook<boost::intrusive::link_mode<
            boost::intrusive::auto_unlink>> SetItem;
    SetItem m_item;

    friend class BusId;

    BusId m_id;
};

#endif /* __SUBSCRIBER_H__ */
