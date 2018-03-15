// Copyright (c) 2018 LG Electronics, Inc.
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

#include "base/LunaURL.h"

#include <gtest/gtest.h>
#include <pbnjson.hpp>

using namespace pbnjson;
using namespace std;

class UnittestMojoUrl : public testing::Test {
protected:
    UnittestMojoUrl()
    {
    }

    virtual ~UnittestMojoUrl()
    {
    }

    void thenValidURL(LunaURL& lunaURL)
    {
        EXPECT_STREQ(URL.c_str(), lunaURL.getString().c_str());
        EXPECT_STREQ(METHOD.c_str(), lunaURL.getMethod().data());
        EXPECT_STREQ(SERVICE.c_str(), lunaURL.getTargetService().data());
    }

    const string SERVICE = "com.webos.service.config";
    const string METHOD = "getConfigs";
    const string URL = "luna://" + SERVICE + "/" + METHOD;
};

TEST_F(UnittestMojoUrl, EmpryUrl)
{
    LunaURL lunaURL;

    EXPECT_STREQ(lunaURL.getString().c_str(), "");
}

TEST_F(UnittestMojoUrl, ConstCharacterURL)
{
    LunaURL lunaURL(URL.c_str());

    thenValidURL(lunaURL);
}

TEST_F(UnittestMojoUrl, MojoStrURL)
{
    MojString mojString;
    mojString.assign(URL.c_str());

    LunaURL lunaURL(mojString);

    thenValidURL(lunaURL);
}

TEST_F(UnittestMojoUrl, TargetServiceAndMethodMojoStrURL)
{
    MojString targetService, targetMethod;

    targetService.assign(SERVICE.c_str());
    targetMethod.assign(METHOD.c_str());

    LunaURL lunaURL(targetService, targetMethod);

    thenValidURL(lunaURL);
}

TEST_F(UnittestMojoUrl, AssignConstCharacters)
{
    LunaURL lunaURL = URL.c_str();

    thenValidURL(lunaURL);
}

TEST_F(UnittestMojoUrl, AssignMojString)
{
    MojString mojString;
    mojString.assign(URL.c_str());
    LunaURL lunaURL = mojString;

    thenValidURL(lunaURL);
}

TEST_F(UnittestMojoUrl, Comparison)
{
    LunaURL A(URL.c_str()), B;

    EXPECT_TRUE(A != B);
    EXPECT_FALSE(A == B);

    B = URL.c_str();

    EXPECT_TRUE(A == B);
    EXPECT_FALSE(A != B);

    B = "luna://com.webos.service.invalid/wrongmethod";

    EXPECT_TRUE(A != B);
    EXPECT_FALSE(A == B);
}
