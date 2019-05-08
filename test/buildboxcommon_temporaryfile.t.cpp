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

#include <buildboxcommon_temporarydirectory.h>

using namespace buildboxcommon;

TEST(TemporaryFileTests, TemporaryFile)
{
    std::string file_name;
    {
        TemporaryFile tempFile;
        file_name = std::string(tempFile.name());

        // Verify that the file exists and is a file.
        struct stat statResult;
        ASSERT_EQ(stat(file_name.c_str(), &statResult), 0);
        ASSERT_TRUE(S_ISREG(statResult.st_mode));

        // It was created inside the default directory:
        EXPECT_NE(file_name.find(TemporaryFileDefaults::DEFAULT_TMP_DIR),
                  std::string::npos);
    }

    // Verify that the file no longer exists.
    struct stat statResult;
    ASSERT_NE(stat(file_name.c_str(), &statResult), 0);
}

TEST(TemporaryFileTests, TemporaryFileWithCustomPrefix)
{
    std::string file_name;
    {
        TemporaryFile tempFile("test-prefix");
        file_name = std::string(tempFile.name());
        EXPECT_NE(file_name.find("test-prefix"), std::string::npos);

        // Verify that the file exists and is a file.
        struct stat statResult;
        ASSERT_EQ(stat(file_name.c_str(), &statResult), 0);
        ASSERT_TRUE(S_ISREG(statResult.st_mode));
    }

    // Verify that the directory no longer exists.
    struct stat statResult;
    ASSERT_NE(stat(file_name.c_str(), &statResult), 0);
}

TEST(TemporaryFileTests, TemporaryFileInGivenDirectory)
{

    std::string file_name;

    // We create a temporary directory in which to place the file:
    TemporaryDirectory directory;

    {
        const std::string prefix = "prefix123";

        TemporaryFile tempFile(directory.name(), prefix.c_str());

        file_name = std::string(tempFile.name());
        EXPECT_NE(file_name.find(prefix), std::string::npos);

        // The file is stored in the directory we wanted:
        const std::string directory_name = std::string(tempFile.name());
        EXPECT_NE(directory_name.find(directory.name()), std::string::npos);

        // Verify that the file exists and is a file.
        struct stat statResult;
        ASSERT_EQ(stat(file_name.c_str(), &statResult), 0);
        ASSERT_TRUE(S_ISREG(statResult.st_mode));
    }

    // Verify that the file no longer exists.
    struct stat statResultFile;
    ASSERT_NE(stat(file_name.c_str(), &statResultFile), 0);

    // But the directory does:
    struct stat statResultDir;
    ASSERT_EQ(stat(directory.name(), &statResultDir), 0);
    ASSERT_TRUE(S_ISDIR(statResultDir.st_mode));
}

TEST(TemporaryFileTests, TemporaryFileInGivenDirectoryWithEmptyPrefix)
{

    std::string file_name;

    // We create a temporary directory in which to place the file:
    TemporaryDirectory directory;

    {
        TemporaryFile tempFile(directory.name(), "");

        file_name = std::string(tempFile.name());

        // The file is stored in the directory we wanted:
        const std::string directory_name = std::string(tempFile.name());
        EXPECT_NE(directory_name.find(directory.name()), std::string::npos);

        // Verify that the file exists and is a file.
        struct stat statResult;
        ASSERT_EQ(stat(file_name.c_str(), &statResult), 0);
        ASSERT_TRUE(S_ISREG(statResult.st_mode));
    }

    // Verify that the file no longer exists.
    struct stat statResultFile;
    ASSERT_NE(stat(file_name.c_str(), &statResultFile), 0);
}
