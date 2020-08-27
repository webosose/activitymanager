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

include(FindPkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LS2 REQUIRED luna-service2)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

find_package(Boost REQUIRED COMPONENTS regex)
include_directories(${Boost_INCLUDE_DIRS})

pkg_check_modules(DB8 REQUIRED db8>=3)
include_directories(${DB8_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${DB8_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS_OTHER})

if (DEFINED INIT_MANAGER)
    # add flag for ifdef
    webos_add_compiler_flags(ALL -DINIT_MANAGER_${INIT_MANAGER})
    # for systemd
    if (${INIT_MANAGER} STREQUAL "systemd")
        # -- check for libsystemd
        pkg_check_modules(LIBINITMGR REQUIRED libsystemd)
        webos_add_compiler_flags(ALL ${LIBINITMGR_CFLAGS})
    endif()
endif()

pkg_check_modules(PMLOG REQUIRED PmLogLib>=3)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src/common)
include_directories(${PROJECT_BINARY_DIR}/Configured/src/common)
webos_add_compiler_flags(ALL -Wall -Wno-error=strict-aliasing -Wconversion -DMOJ_LINUX -DBOOST_SP_DISABLE_THREADS -DUSE_PMLOG -std=c++11)
webos_add_compiler_flags(DEBUG -DMOJ_DEBUG)

# MojoDb generates warnings in core/MojObject.h, so no -Werror
#webos_add_compiler_flags(ALL -Werror )

webos_add_linker_options (ALL "--no-undefined")
#Uncomment following line to see Trace logs
#add_definitions(-DPMLOG_TRACES_ENABLED -DPMLOG_TRACE_COMPONENT="ActivityManager")

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE SRC_COMMON ${PROJECT_SOURCE_DIR}/src/common/*.cpp)
add_executable(activitymanager ${SRC_COMMON} ${SRC_FILES})

set(LIBS ${DB8_LDFLAGS}
         ${Boost_LIBRARIES}
         ${GLIB2_LDFLAGS}
         ${LS2_LDFLAGS}
         ${PMLOG_LDFLAGS}
         ${PBNJSON_CPP_LDFLAGS}
         ${LIBINITMGR_LDFLAGS}
)
target_link_libraries(activitymanager ${LIBS})

webos_build_daemon()
