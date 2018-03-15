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

#ifndef __CATEGORY_H__
#define __CATEGORY_H__

#define ACTIVITY_SERVICEMETHOD_BEGIN() try { do {} while (0) \

#define ACTIVITY_SERVICEMETHOD_END(serviceMsg) \
} catch (const std::exception& except) { \
    MojErr err = (serviceMsg)->replyError(MojErrInternal, except.what()); \
    MojErrCheck(err); \
    return MojErrNone; \
} catch (...) { \
    MojErr err = (serviceMsg)->replyError(MojErrInternal, \
        "Internal exception of unknown type thrown"); \
    MojErrCheck(err); \
    return MojErrNone; \
}

#define ACTIVITY_SERVICEMETHODFINISH_END(serviceMsg) \
} catch (const std::exception& except) { \
    (void) (serviceMsg)->replyError(MojErrInternal, except.what()); \
    return; \
} catch (...) { \
    (void) (serviceMsg)->replyError(MojErrInternal, \
        "Internal exception of unknown type thrown"); \
    return; \
}

#endif /* __CATEGORY_H__ */
