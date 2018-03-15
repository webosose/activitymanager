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

#ifndef __ACTIVITY_SERVICE_SCHEMAS_H__
#define __ACTIVITY_SERVICE_SCHEMAS_H__

#define CALLBACK_TYPE_OBJECT_SCHEMA \
    _T("\"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"method\": { \"type\": \"string\" } ") \
        _T("}") \

#define TRIGGER_TYPE_OBJECT_SCHEMA \
    _T("\"type\": \"object\" ")

#define METADATA_TYPE_OBJECT_SCHEMA \
    _T("\"type\": \"object\" ")

#define SCHEDULE_TYPE_OBJECT_SCHEMA \
    _T("\"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"precise\": { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"start\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"interval\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"skip\": { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"local\": { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"end\": { \"type\": \"string\", \"optional\": true }, ") \
            _T(" \"relative\": { \"type\": \"boolean\", \"optional\": true }, ") \
            _T(" \"lastFinished\": { \"type\": \"string\", \"optional\": true } ") \
        _T("}")

#define CALLBACK_TYPE_SCHEMA \
    _T("\"type\": [") \
        _T("{ \"type\": \"boolean\" }, ") \
        _T("{ ") CALLBACK_TYPE_OBJECT_SCHEMA _T("} ") \
    _T("]")

#define TRIGGER_TYPE_SCHEMA \
    _T("\"type\": [") \
        _T("{ \"type\": \"boolean\" }, ") \
        _T("{ ") TRIGGER_TYPE_OBJECT_SCHEMA _T("} ") \
    _T("]")

#define METADATA_TYPE_SCHEMA \
    _T("\"type\": [") \
        _T("{ \"type\": \"boolean\" }, ") \
        _T("{ ") METADATA_TYPE_OBJECT_SCHEMA _T("} ") \
    _T("]")

#define SCHEDULE_TYPE_SCHEMA \
    _T("\"type\": [") \
        _T("{ \"type\": \"boolean\" }, ") \
        _T("{ ") SCHEDULE_TYPE_OBJECT_SCHEMA _T("} ") \
    _T("]")

#define ACTIVITY_TYPE_SCHEMA \
    _T("\"type\": \"object\", ") \
        _T(" \"properties\": { ") \
            _T(" \"name\": { \"type\": \"string\" }, ") \
            _T(" \"description\": { \"type\": \"string\" } ") \
        _T("}")

#endif /* __ACTIVITY_SERVICE_SCHEMAS_H__ */
