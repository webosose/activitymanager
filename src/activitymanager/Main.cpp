// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ActivityManagerApp.h"

static long original_timezone_offset;

static void switch_to_utc()
{
    /* Make sure the time globals are set */
    tzset();

    /* Determine current timezone.  Whether to use daylight time can be
     * found in the 'daylight' global.  (glibc isn't computing it automatically
     * if you set tm_isdst to -1.  Ouch.) */

    /* Time can shift backwards or forwards... put 24 hours on the clock! */
    time_t base_local_time = 24*60*60;

    struct tm utctm;
    memset(&utctm, 0, sizeof(tm));

    /* No conversions... mktime will do that */
    gmtime_r(&base_local_time, &utctm);

    /* -1 does not appear to work under test, but daylight is set properly */
    utctm.tm_isdst = daylight;

    /* When local time was 'base_local_time', UTC was... */
    time_t utc_time = mktime(&utctm);

    /* Store original timezone offset */
    original_timezone_offset = (base_local_time - utc_time);

    /* Now set time processing to happen in UTC. */
    /* Since almost everything already is, this basically just fixes
     * mktime(3) */
    setenv("TZ", "", 1);
    tzset();
}

int main(int argc, char** argv)
{
    switch_to_utc();

    ActivityManagerApp app;
    int mainResult = app.main(argc, argv);

    return mainResult;
}
