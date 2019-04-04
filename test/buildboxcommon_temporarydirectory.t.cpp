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

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_temporarydirectory.h>
#include <fstream>
#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(TemporaryDirectoryTests, TemporaryDirectory)
{
    std::string name;
    {
        TemporaryDirectory tempDir = TemporaryDirectory("test-prefix");
        name = std::string(tempDir.name());
        EXPECT_NE(name.find("test-prefix"), std::string::npos);

        // Verify that the directory exists and is a directory.
        struct stat statResult;
        ASSERT_EQ(stat(name.c_str(), &statResult), 0);
        ASSERT_TRUE(S_ISDIR(statResult.st_mode));
    }

    // Verify that the directory no longer exists.
    struct stat statResult;
    ASSERT_NE(stat(name.c_str(), &statResult), 0);
}

TEST(TemporaryDirectoryTests, CreateDeleteDirectory)
{
    TemporaryDirectory tempDir = TemporaryDirectory();
    std::string name = tempDir.name() + std::string("/some/directory/path");

    ASSERT_THROW(FileUtils::delete_directory(name.c_str()), std::exception);

    FileUtils::create_directory(name.c_str());

    // Verify that the directory exists and is a directory.
    struct stat statResult;
    ASSERT_EQ(stat(name.c_str(), &statResult), 0);
    ASSERT_TRUE(S_ISDIR(statResult.st_mode));

    name = tempDir.name() + std::string("/some");

    FileUtils::delete_directory(name.c_str());

    ASSERT_NE(stat(name.c_str(), &statResult), 0);
}

TEST(TemporaryDirectoryTests, ExecutableFlag)
{
    TemporaryDirectory tempDir = TemporaryDirectory();
    std::string name = tempDir.name() + std::string("/test.py");

    EXPECT_FALSE(FileUtils::is_executable(name.c_str()));
    ASSERT_THROW(FileUtils::make_executable(name.c_str()), std::exception);

    std::ofstream fileStream(name);
    fileStream << "#!/usr/bin/env python3" << std::endl;
    fileStream.close();

    // Verify that the file exists and is not executable (yet).
    struct stat statResult;
    ASSERT_EQ(stat(name.c_str(), &statResult), 0);
    EXPECT_FALSE(FileUtils::is_executable(name.c_str()));

    FileUtils::make_executable(name.c_str());

    // Verify that the file now is executable.
    EXPECT_TRUE(FileUtils::is_executable(name.c_str()));
}
