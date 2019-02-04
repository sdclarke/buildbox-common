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

#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

using namespace buildboxcommon;

TEST(FileUtilsTest, TemporaryFile)
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

TEST(FileUtilsTest, TemporaryDirectory)
{
    std::string name;
    {
        TemporaryDirectory tempDir("test-prefix");
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

TEST(FileUtilsTest, CreateDeleteDirectory)
{
    TemporaryDirectory tempDir;
    std::string name = tempDir.name() + std::string("/some/directory/path");

    ASSERT_THROW(delete_directory(name.c_str()), std::exception);

    create_directory(name.c_str());

    // Verify that the directory exists and is a directory.
    struct stat statResult;
    ASSERT_EQ(stat(name.c_str(), &statResult), 0);
    ASSERT_TRUE(S_ISDIR(statResult.st_mode));

    name = tempDir.name() + std::string("/some");

    delete_directory(name.c_str());

    ASSERT_NE(stat(name.c_str(), &statResult), 0);
}

TEST(FileUtilsTest, ExecutableFlag)
{
    TemporaryDirectory tempDir;
    std::string name = tempDir.name() + std::string("/test.py");

    ASSERT_THROW(is_executable(name.c_str()), std::exception);
    ASSERT_THROW(make_executable(name.c_str()), std::exception);

    std::ofstream fileStream(name);
    fileStream << "#!/usr/bin/env python3" << std::endl;
    fileStream.close();

    // Verify that the file exists and is not executable (yet).
    struct stat statResult;
    ASSERT_EQ(stat(name.c_str(), &statResult), 0);
    EXPECT_FALSE(is_executable(name.c_str()));

    make_executable(name.c_str());

    // Verify that the file now is executable.
    EXPECT_TRUE(is_executable(name.c_str()));
}
