// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <core/MojObject.h>
#include <pbnjson.hpp>

#include "base/IStringify.h"
#include "util/MojoObjectJson.h"

struct RequirementInfo : public IStringify {
    std::string name;
    std::string method;
    MojObject params;
    MojObject where;
    MojObject type;

    MojErr toJson(MojObject& reply) const {
        MojErr err = MojErrNone;

        err = reply.putString("name", name.c_str());
        MojErrCheck(err);

        err = reply.put("type-schema", type);
        MojErrCheck(err);

        return MojErrNone;
    }

    virtual pbnjson::JValue toJson() const {
        pbnjson::JValue obj = pbnjson::Object();
        obj.put("name", name);
        obj.put("type-schema", MojoObjectJson::convertMojObjToJValue(type));
        return obj;
    }
};

class Config {
public:
    static Config& getInstance();

    static const char *kMonitorSocketAddress;

    void load(std::string filename, bool append = true);

    unsigned int getFailedLimitCount() const;
    int getRestartLimitCount() const;
    double getRestartLimitInterval() const;

    /** should check null */
    std::shared_ptr<RequirementInfo> getRequirement(const std::string& name) const;
    std::list<std::shared_ptr<RequirementInfo>> getRequirements() const;

    bool validateCallerEnabled() const;

private:
    Config();
    Config(const Config&) = delete;
    virtual ~Config();

    static MojObject convertToMojObject(pbnjson::JValue&& jvalue);

    void clear();

    unsigned int m_failedLimitCount;
    int m_restartLimitCount;
    double m_restartLimitInterval;
    std::list<std::shared_ptr<RequirementInfo>> m_requirements;
    bool m_validateCallerEnabled;
};

#endif /* __CONFIG_H__ */
