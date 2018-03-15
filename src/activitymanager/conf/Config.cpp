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

#include <conf/Environment.h>
#include "Config.h"

#include <algorithm>

#include <core/MojJson.h>
#include <core/MojObjectBuilder.h>
#include <pbnjson.hpp>

#include "util/Logging.h"
#include "util/MojoObjectJson.h"
#include "util/SchemaResolver.h"

Config::Config()
    : m_failedLimitCount(0)
    , m_restartLimitCount(0)
    , m_restartLimitInterval(0)
    , m_validateCallerEnabled(true)
{
    load(CONFIG_BASE_PATH, false);
}

Config::~Config()
{
}

Config& Config::getInstance()
{
    static Config config;
    return config;
}

void Config::clear()
{
    m_requirements.clear();
}

void Config::load(std::string filename, bool append)
{
    LOG_AM_INFO(MSGID_CONFIG_LOAD, 2,
                PMLOGKS("file", filename.c_str()),
                PMLOGKFV("append", "%d", append), "");

    if (!append) {
        clear();
    }

    pbnjson::JSchema schema = pbnjson::JSchema::fromFile(SCHEMA_ACTIVITYMANAGER_PATH);
    if (!schema.isInitialized()) {
        LOG_AM_WARNING(MSGID_CONFIG_LOAD_FAIL, 0,
                       "Failed to parse schema %s ", SCHEMA_ACTIVITYMANAGER_PATH);
        return;
    }

    SchemaResolver resolver(SCHEMA_DIR);
    schema.resolve(resolver);

    pbnjson::JValue root = pbnjson::JDomParser::fromFile(filename.c_str(), schema);
    if (resolver.isError()) {
        LOG_AM_WARNING(MSGID_CONFIG_LOAD_FAIL, 0, "%s", resolver.errorString().c_str());
        return;
    }
    if (!root.isValid() || root.isNull()) {
        LOG_AM_WARNING(MSGID_CONFIG_LOAD_FAIL, 0, "Failed to parse config %s", filename.c_str());
        return;
    }

    if (root.hasKey("common")) {
        pbnjson::JValue common = root["common"];
        if (common.hasKey("failed-limit")) {
            pbnjson::JValue failedLimit = common["failed-limit"];
            int failedLimitCount = failedLimit["count"].asNumber<int32_t>();
            if (failedLimitCount >= 0) {
                m_failedLimitCount = failedLimitCount;
            }
        }

        if (common.hasKey("restart-limit")) {
            pbnjson::JValue restartLimit = common["restart-limit"];
            m_restartLimitCount = restartLimit["count"].asNumber<int32_t>();
            m_restartLimitInterval = restartLimit["interval-ms"].asNumber<int32_t>() / 1000.0;
        }

        if (common.hasKey("validate-caller")) {
            m_validateCallerEnabled = common["validate-caller"].asBool();
        }
    }

    pbnjson::JValue requirements = root["requirements"];
    for (int i = 0; i < requirements.arraySize(); ++i) {
        pbnjson::JValue requirement = requirements[i];

        std::shared_ptr<RequirementInfo> info = std::make_shared<RequirementInfo>();
        info->name = requirement["name"].asString();
        info->method = requirement["method"].asString();
        info->params = convertToMojObject(requirement["params"]);
        info->where =  convertToMojObject(requirement["where"]);
        info->type =  convertToMojObject(requirement["type-schema"]);

        std::shared_ptr<RequirementInfo> oldInfo = getRequirement(info->name);
        if (oldInfo) {
            m_requirements.remove(oldInfo);
            m_requirements.push_back(info);
        } else {
            m_requirements.push_back(info);
        }

        LOG_AM_INFO(MSGID_CONFIG_LOAD_REQ, 4,
                    PMLOGKS("name", info->name.c_str()),
                    PMLOGKS("method", info->method.c_str()),
                    PMLOGJSON("params", MojoObjectJson(info->params).c_str()),
                    PMLOGJSON("where", MojoObjectJson(info->where).c_str()), "");
    }
}

std::shared_ptr<RequirementInfo> Config::getRequirement(const std::string& name) const
{
    auto it = std::find_if(
            m_requirements.begin(),
            m_requirements.end(),
            [&] (const std::shared_ptr<RequirementInfo>& info) { return info->name == name; });

    if (it != m_requirements.end()) {
        return (*it);
    } else {
        return nullptr;
    }
}

std::list<std::shared_ptr<RequirementInfo>> Config::getRequirements() const
{
    return m_requirements;
}

MojObject Config::convertToMojObject(pbnjson::JValue&& jvalue)
{
    MojObjectBuilder builder;
    MojErr err = MojJsonParser::parse(builder, jvalue.stringify().c_str());

    if (err == MojErrNone) {
        return builder.object();
    } else {
        return MojObject::Undefined;
    }
}

unsigned int Config::getFailedLimitCount() const
{
    return m_failedLimitCount;
}

int Config::getRestartLimitCount() const
{
    return m_restartLimitCount;
}

double Config::getRestartLimitInterval() const
{
    return m_restartLimitInterval;
}

bool Config::validateCallerEnabled() const
{
    return m_validateCallerEnabled;
}
