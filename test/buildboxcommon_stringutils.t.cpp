/*
 * Copyright 2020 Bloomberg Finance LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <buildboxcommon_stringutils.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;
using namespace testing;

int myCustomFilter(int ch) { return (ch == '@' || std::isspace(ch)); }

// TEST CASES
TEST(StringUtilsTest, LeftTrimTest)
{
    std::string inputString(" \t\ntesting one two three");
    std::string expectedString("testing one two three");
    StringUtils::ltrim(&inputString);
    EXPECT_EQ(inputString, expectedString);
}

TEST(StringUtilsTest, LeftTrimFailTest)
{
    std::string inputString(" \t\ntesting one two three");
    std::string expectedString(" testing one two three");
    StringUtils::ltrim(&inputString);
    EXPECT_NE(inputString, expectedString);
}

TEST(StringUtilsTest, RightTrimTest)
{
    std::string inputString("testing one two three \t\n");
    std::string expectedString("testing one two three");
    StringUtils::rtrim(&inputString);
    EXPECT_EQ(inputString, expectedString);
}

TEST(StringUtilsTest, TrimTest)
{
    std::string inputString(" \t\ntesting one two three \t\n");
    std::string expectedString("testing one two three");
    StringUtils::trim(&inputString);
    EXPECT_EQ(inputString, expectedString);
}

TEST(StringUtilsTest, LeftTrimTestCopy)
{
    std::string inputString(" \t\ntesting one two three");
    std::string expectedString("testing one two three");
    EXPECT_EQ(StringUtils::ltrim(inputString), expectedString);
}

TEST(StringUtilsTest, RightTrimTestCopy)
{
    std::string inputString("testing one two three \t\n");
    std::string expectedString("testing one two three");
    EXPECT_EQ(StringUtils::rtrim(inputString), expectedString);
}

TEST(StringUtilsTest, TrimTestCopy)
{
    std::string inputString(" \t\ntesting one two three \t\n");
    std::string expectedString("testing one two three");
    EXPECT_EQ(StringUtils::trim(inputString), expectedString);
}

TEST(StringUtilsTest, LeftTrimCustomSuccessTest)
{
    std::string inputString("@ \n\ttesting one two three");
    std::string expectedString("testing one two three");
    StringUtils::ltrim(&inputString, myCustomFilter);
    EXPECT_EQ(inputString, expectedString);
}

TEST(StringUtilsTest, LeftTrimCustomFailTest)
{
    std::string inputString("! \n\ttesting one two three");
    std::string expectedString("! \n\ttesting one two three");
    StringUtils::ltrim(&inputString, myCustomFilter);
    EXPECT_EQ(inputString, expectedString);
}
