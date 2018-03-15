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

#ifndef __MAIN_H__
#define __MAIN_H__

/* Check to make sure minimum capabilities are present in Boost, and enable */
#include <boost/version.hpp>

#if !defined(BOOST_VERSION) || (BOOST_VERSION < 103900)
#error Boost prior to 1.39.0 contains a critical bug in the smart_ptr class
#endif

#include <memory>
#include <functional>

/* Boost bind library - defines mem_fn (like std::mem_fun, but can handle
 * smart pointers as well as references).  Also more flexible bind semantics */
#include <boost/bind.hpp>

#include <stdint.h>

typedef unsigned long long activityId_t;

#include <errno.h>
#include <core/MojString.h>
#include <core/MojErr.h>

#include "util/Logging.h"
#include "util/MojoObjectJson.h"

#endif /* __MAIN_H__ */
