/*
 * Copyright 2018 Bloomberg Finance LP
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

#include <buildboxcommon_temporaryfile.h>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

using namespace buildboxcommon;

TEST(TemporaryFileTests, TemporaryFile)
{
    std::string name;
    {
        TemporaryFile tempFile("test-prefix");
        name = std::string(tempFile.name());
        EXPECT_NE(name.find("test-prefix"), std::string::npos);

        // Verify that the file exists and is a file.
        struct stat statResult;
        ASSERT_EQ(stat(name.c_str(), &statResult), 0);
        ASSERT_TRUE(S_ISREG(statResult.st_mode));
    }

    // Verify that the directory no longer exists.
    struct stat statResult;
    ASSERT_NE(stat(name.c_str(), &statResult), 0);
}
