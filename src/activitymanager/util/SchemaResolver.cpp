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

#include "SchemaResolver.h"

#include <fstream>

SchemaResolver::SchemaResolver(const char *schemaPath)
    : m_schemaPath(schemaPath)
{
}

SchemaResolver::~SchemaResolver()
{
}

pbnjson::JSchema SchemaResolver::resolve(
        const ResolutionRequest& request, JSchemaResolutionResult& result)
{
    std::string schemaFile = m_schemaPath + "/" + request.resource() + ".schema";

    pbnjson::JSchema schema = pbnjson::JSchema::fromFile(schemaFile.c_str());
    if (!schema.isInitialized()) {
        m_errorString = "Failed to resolve schema " + schemaFile;
        result = SCHEMA_INVALID;
        return pbnjson::JSchema::NullSchema();
    }

    result = SCHEMA_RESOLVED;
    return schema;
}

bool SchemaResolver::isError()
{
    return !m_errorString.empty();
}

std::string SchemaResolver::errorString()
{
    return m_errorString;
}
