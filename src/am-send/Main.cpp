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
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <string>

#include <glib.h>
#include <pbnjson.hpp>

#include "conf/Setting.h"

static gint64 option_id = 0;
static gchar **option_requirement = NULL;
static gchar **option_trigger = NULL;
static gboolean option_clear = FALSE;

static const gchar *OPTION_DESCRIPTION =
        "am-send simulates specified activity's conditions are satisfied.\n"
        "Simulate 'wifi' and 'bootup' requirements are satisfied:\n"
        "$ am-send --id 56 -r wifi -r bootup\n"
        "Simulate 'wifi' requirement to unsatisfy:\n"
        "$ am-send --id 56 -r '{\"wifi\":false}'\n"
        "Unset user-defined conditions for 'wifi' and 'bootup' requirements:\n"
        "$ am-send --id 56 -r wifi -r bootup --clear\n"
        "Simulate 't1' and 't2' triggers are satisfied:\n"
        "$ am-send --id 56 -t t1 -t t2\n"
        "Simulate 't3' is unsatisfied and 't4' is satisfied:\n"
        "$ am-send --id 56 -t '{\"t3\":false}' -t '{\"t4\":true}'\n";

static GOptionEntry OPTION_ENTRIES[] = {
    {
        "id", 0, 0, G_OPTION_ARG_INT, &option_id,
        "Activity ID to simulate", "56"
    },
    {
        "requirement", 'r', 0, G_OPTION_ARG_STRING_ARRAY, &option_requirement,
        "Requirement name to simulate", "wifi"
    },
    {
        "trigger", 't', 0, G_OPTION_ARG_STRING_ARRAY, &option_trigger,
        "Trigger name to simulate", "triggerName"
    },
    {
         "clear", 0, 0, G_OPTION_ARG_NONE, &option_clear,
         "Clear user-defined conditions", NULL
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
    int pipeReq = 0;
    int pipeResp = 0;
    pbnjson::JValue json = pbnjson::Object();
    std::string request;
    std::ifstream file;
    std::string line;

    g_option_context_add_main_entries (context, OPTION_ENTRIES, NULL);
    g_option_context_set_description(context, OPTION_DESCRIPTION);
    if (!g_option_context_parse(context, &argc, &argv, &gerror)) {
        std::cerr << "Option parsing error: " << gerror->message << std::endl;
        exitcode = -1;
        goto Exit;
    }

    if (option_id > 0) {
        json.put("id", option_id);
    } else {
        std::cerr << "ID parsing error: id is required" << std::endl;
        exitcode = -1;
        goto Exit;
    }

    if (option_clear) {
        json.put("clear", option_clear);
    }

    if (option_requirement) {
        pbnjson::JValue requirements = pbnjson::Array();
        int option_size = g_strv_length(option_requirement);
        for (int i = 0; i < option_size; i++) {
            pbnjson::JValue requirement = pbnjson::JDomParser::fromString(option_requirement[i]);
            if (requirement.isNull()) {
                requirement = pbnjson::Object();
                requirement.put(option_requirement[i], true);
                requirements.append(requirement);
            } else if (requirement.isObject()) {
                requirements.append(requirement);
            } else {
                std::cerr << "Requirement parsing error: " << option_requirement[i] << std::endl;
                exitcode = -1;
                goto Exit;
            }
        }
        json.put("requirements", requirements);
    }

    if (option_trigger) {
        pbnjson::JValue triggers = pbnjson::Array();
        int option_size = g_strv_length(option_trigger);
        for (int i = 0; i < option_size; i++) {
            pbnjson::JValue trigger = pbnjson::JDomParser::fromString(option_trigger[i]);
            if (trigger.isNull()) {
                trigger = pbnjson::Object();
                trigger.put(option_trigger[i], true);
                triggers.append(trigger);
            } else if (trigger.isObject()) {
                triggers.append(trigger);
            } else {
                std::cerr << "Trigger parsing error: " << option_trigger[i] << std::endl;
                exitcode = -1;
                goto Exit;
            }
        }
        json.put("triggers", triggers);
    }

    request = json.stringify() + "\n";

    if (g_mkdir_with_parents(AM_IPC_DIR, 0755) == -1) {
        std::cerr << "Failed to create dir: " << strerror(errno) << std::endl;
        exitcode = errno;
        goto Exit;
    }

    if ((pipeReq = open(AM_SEND_REQ_PIPE_PATH, O_WRONLY)) == -1) {
        std::cerr << "Failed to open request pipe: " << strerror(errno) << std::endl;
        exitcode = errno;
        goto Exit;
    }

    if (write(pipeReq, request.c_str(), request.length()) == -1) {
        std::cerr << "Failed to write data: " << strerror(errno) << std::endl;
        exitcode = errno;
        goto Exit;
    }

    file.open(AM_SEND_RESP_PIPE_PATH);
    try {
        std::getline(file, line);
    } catch (std::bad_cast& e) {
        std::cerr << "bad_cast error while reading from pipe: " << line << std::endl;
    }
    std::cout << line << std::endl;

Exit:
    if (option_requirement) {
        g_strfreev(option_requirement);
    }
    if (option_trigger) {
        g_strfreev(option_trigger);
    }
    if (context) {
        g_option_context_free(context);
    }

    if (pipeReq >= 0) {
        close(pipeReq);
    }
    if (pipeResp >= 0) {
        close(pipeResp);
    }

    return exitcode;
}

