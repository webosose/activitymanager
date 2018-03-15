# Copyright (c) 2009-2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)

project(activitymanager CXX)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC2)
webos_component(3 0 0)

webos_machine_impl_dep()

webos_build_system_bus_files()
webos_build_db8_files()

if (WEBOS_CONFIG_BUILD_TESTS)
    webos_use_gtest()
    webos_add_compiler_flags(ALL -DUNITTEST)
    add_subdirectory(unit_test)
endif()

add_subdirectory(src/common)
add_subdirectory(src/activitymanager)
add_subdirectory(src/am-monitor)
add_subdirectory(src/am-send)

file(GLOB_RECURSE SCHEMAS files/schema/*.schema)
install(FILES ${SCHEMAS} DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/schemas/activitymanager)
file(GLOB_RECURSE CONFIGS files/conf/*)
install(FILES ${CONFIGS} DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR})
