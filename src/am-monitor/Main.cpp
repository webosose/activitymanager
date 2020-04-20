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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <fstream>
#include <iostream>
#include <string>

#include <glib.h>
#include <pbnjson.hpp>

#include "conf/Setting.h"

static gboolean option_list = FALSE;
static gboolean option_debug = FALSE;
static gboolean option_json = FALSE;
static gboolean option_compact = FALSE;
static gboolean option_pretty_format = FALSE;

static gint64 option_id = 0;
static gchar *option_name = NULL;

static const gchar *OPTION_DESCRIPTION =
        "am-monitor supports test automation and debugging.\n"
        "Print state changes for specified activity name:\n"
        "$ am-monitor --name=\"update check activity\"\n"
        "Print state changes in json format for specified activity id:\n"
        "$ am-monitor --id=56 --json --compact\n"
        "Print all activities:\n"
        "$ am-monitor --list\n";

static GOptionEntry OPTION_ENTRIES[] = {
    {
        "list", 0, 0, G_OPTION_ARG_NONE, &option_list,
        "List all activities"
    },
    {
        "debug", 0, 0, G_OPTION_ARG_NONE, &option_debug,
        "Print extra output for debugging"
    },
    {
        "json", 0, 0, G_OPTION_ARG_NONE, &option_json,
        "Print JSON formatted output"
    },
    {
        "id", 0, 0, G_OPTION_ARG_INT64, &option_id,
        "Filter by activity ID", "56"
    },
    {
        "name", 0, 0, G_OPTION_ARG_STRING, &option_name,
        "Filter by activity name", "\"update check\""
    },
    {
        "compact", 0, 0, G_OPTION_ARG_NONE, &option_compact,
        "Print compact output to fit terminal"
    },
    {
        "pretty-format", 0, 0, G_OPTION_ARG_NONE, &option_pretty_format,
        "Format JSON response with good readability"
    },
    {
        NULL
    }
};

int main(int argc, char *argv[])
{
    int exitcode = 0;
    GOptionContext* context = g_option_context_new(NULL);
    GError *gerror = NULL;
    int lock;
    int pipeReq;
    std::string command;
    std::ifstream file;
    std::string line;
    struct timespec now;

    g_option_context_add_main_entries (context, OPTION_ENTRIES, NULL);
    g_option_context_set_description(context, OPTION_DESCRIPTION);
    if (!g_option_context_parse(context, &argc, &argv, &gerror)) {
        std::cerr << "Option parsing error: " << gerror->message << std::endl;
        exitcode = -1;
        goto Exit;
    }

    if (g_mkdir_with_parents(AM_IPC_DIR, 0755) == -1) {
        std::cerr << "Failed to create dir: " << strerror(errno) << std::endl;
        exitcode = errno;
        goto Exit;
    }

    if ((lock = open(AM_MONITOR_LOCK_PATH, O_WRONLY | O_CREAT, 0600)) == -1) {
        std::cerr << "Failed to open lock: " << strerror(errno) << std::endl;
        exitcode = errno;
        goto Exit;
    }

    if (lockf(lock, F_TLOCK, 0) == -1) {
        std::cerr << "Failed to lock: " << strerror(errno) << std::endl;
        exitcode = errno;
        goto Exit;
    }

    if (mkfifo(AM_MONITOR_REQ_PIPE_PATH, 0600) < 0) {
        if (errno != EEXIST) {
            std::cerr << "Failed to create request pipe: " << strerror(errno) << std::endl;
            goto Exit;
        }
    }

    if ((pipeReq = open(AM_MONITOR_REQ_PIPE_PATH, O_WRONLY)) == -1) {
        std::cerr << "Failed to open request pipe: " << strerror(errno) << std::endl;
        goto Exit;
    }

    if (option_list) {
        command = "list\n";
    } else {
        command = "monitor\n";
    }

    if (write(pipeReq, command.c_str(), command.length()) == -1) {
        std::cerr << "Failed to write request pipe: " << strerror(errno) << std::endl;
        goto Exit;
    }

    file.open(AM_MONITOR_RESP_PIPE_PATH);
    while (std::getline(file, line, '\n')) {
        pbnjson::JValue root = pbnjson::JDomParser::fromString(line);
        if (!root.isValid() || root.isNull()) {
            exitcode = -1;
            goto Exit;
        }

        if (option_id != 0) {
            int64_t activityId = root["activityId"].asNumber<int64_t>();
            if (activityId != option_id) {
                continue;
            }
        }

        if (option_name) {
            std::string activityName = root["name"].asString();
            std::size_t found =  activityName.find(option_name);
            if (found == std::string::npos) {
                continue;
            }
        }

        if (option_compact) { // TODO
            root.remove("parent");
            root.remove("requirements");
            root.remove("trigger");
        }

        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            std::cerr << "Failed to get time: %s" << strerror(errno) << std::endl;
            exitcode = errno;
            goto Exit;
        }
        double time = ((double)(now.tv_sec)) + (((double)now.tv_nsec) / (double)1000000000.0);

        if (option_json) {
            root.put("time", time);
            if (option_pretty_format) {
                std::cout << root.stringify("    ");
            } else {
                std::cout << root.stringify() << std::endl;
            }
        } else {
            int64_t id = root["activityId"].asNumber<int64_t>();
            std::string name = root["name"].asString();
            std::string state = root["state"].asString();
            std::string command = root["command"].asString();
            std::string parent = root["parent"].asString();
            std::string requirements = ""; // TODO
            std::string trigger = ""; // TODO

            printf("%12.6f \t%6llu \t%s  \t%-50s \t%s\n",
                   time, id, state.c_str(), name.c_str(), parent.c_str());
        }
    }

Exit:
    if (option_name) {
        g_free(option_name);
    }
    if (context) {
        g_option_context_free(context);
    }

    return exitcode;
}

