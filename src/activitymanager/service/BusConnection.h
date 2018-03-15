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

#ifndef _BUS_BUSCONNECTION_H_
#define _BUS_BUSCONNECTION_H_

#include <luna/MojLunaService.h>

class BusConnection {
public:
    static BusConnection& getInstance()
    {
        static BusConnection _instance;
        return _instance;
    }
    virtual ~BusConnection();

    MojLunaService& getService();
    MojLunaService& getClient();

private:
    BusConnection();

    /* The Activity Manager Client is used to handle calls related to the
     * Activity Manager's operation that don't directly associate with
     * an Activity.  All the implementation details use that service handle.
     *
     * Anything an Activity directly causes - Triggers and Callbacks, and
     * of course the incoming calls for the Activities and responses to
     * those calls (and subscriptions) is routed through the main
     * Activity Manager service on the bus. */
    MojLunaService m_client;
    MojLunaService m_service;

};

#endif /* _BUS_BUSCONNECTION_H_ */
