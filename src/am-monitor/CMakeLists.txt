# Copyright (c) 2017-2024 LG Electronics, Inc.
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

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LS2 REQUIRED luna-service2)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

pkg_check_modules(LS2PP REQUIRED luna-service2++)
include_directories(${LS2PP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2PP_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS_OTHER})

# Basic
set(BIN_NAME am-monitor)

# Compiling
webos_add_compiler_flags(ALL --std=c++11)
file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE SRC_COMMON ${PROJECT_SOURCE_DIR}/src/common/*.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src/common)
include_directories(${PROJECT_BINARY_DIR}/Configured/src/common)
add_executable(${BIN_NAME} ${SRC_COMMON} ${SRC_FILES})

# Linking
webos_add_linker_options(ALL --no-undefined)
set(LIBS ${GLIB2_LDFLAGS}
         ${LS2_LDFLAGS}
         ${LS2PP_LDFLAGS}
         ${PBNJSON_CPP_LDFLAGS}
)
target_link_libraries(${BIN_NAME} ${LIBS})

# Install
webos_build_program(NAME ${BIN_NAME})
