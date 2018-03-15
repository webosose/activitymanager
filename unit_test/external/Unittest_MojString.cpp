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

#include <core/MojString.h>

#include <gtest/gtest.h>
#include <pbnjson.hpp>

using namespace pbnjson;
using namespace std;

class UnittestMojString : public testing::Test {
protected:
    UnittestMojString()
    {
    }

    virtual ~UnittestMojString()
    {
    }
};

TEST_F(UnittestMojString, EmpryMojString)
{
    MojString mojString;

    EXPECT_STREQ("", mojString.data());
}

TEST_F(UnittestMojString, AssignMethod)
{
    MojString mojString;

    EXPECT_EQ(MojErrNone, mojString.assign("TEST"));
    EXPECT_STREQ("TEST", mojString.data());
}

