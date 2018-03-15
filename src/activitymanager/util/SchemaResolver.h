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

#ifndef __SCHEMA_RESOLVER_H__
#define __SCHEMA_RESOLVER_H__

#include <string>
#include <pbnjson.hpp>

class SchemaResolver : public pbnjson::JResolver {
public:
    SchemaResolver(const char *schemaPath);
    virtual ~SchemaResolver();

    virtual pbnjson::JSchema resolve(
            const ResolutionRequest& request, JSchemaResolutionResult& result);

    bool isError();
    std::string errorString();

private:
    std::string m_schemaPath;
    std::string m_errorString;
};

#endif /* __SCHEMA_RESOLVER_H__ */
