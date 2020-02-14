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

#include "buildboxcommontest_utils.h"
#include <buildboxcommon_direntwrapper.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_temporarydirectory.h>
#include <buildboxcommon_temporaryfile.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(DirentWrapperTests, CreateAndCheckDirectory)
{
    TemporaryDirectory tempDir = TemporaryDirectory("testdir");
    auto name = std::string(tempDir.name());
    auto subdir =
        buildboxcommontest::TestUtils::createSubDirectory(name, "subdir");

    DirentWrapper d(name);
    ASSERT_EQ(d.path(), name);
    ASSERT_EQ(d.currentEntryPath(), subdir);
}

TEST(DirentWrapperTests, CreateAndCheckSubDirectory)
{
    TemporaryDirectory tempDir = TemporaryDirectory("testdir");
    auto name = std::string(tempDir.name());
    auto subdir =
        buildboxcommontest::TestUtils::createSubDirectory(name, "subdir1");

    DirentWrapper d(name);
    ASSERT_EQ(d.path(), name);
    ASSERT_EQ(d.currentEntryPath(), subdir);
    // increment will call readdir, but since this directory only has one
    // subdirectory, readdir should return nullptr, and currentEntryPath is
    // empty.
    ++d;
    ASSERT_EQ(d.path(), name);
    ASSERT_EQ(d.currentEntryPath(), "");
}

TEST(DirentWrapperTests, CreateEmptyDirectory)
{
    TemporaryDirectory tempDir = TemporaryDirectory("testdir");
    auto name = std::string(tempDir.name());
    FileUtils::createDirectory(name.c_str());

    DirentWrapper d(name);
    ASSERT_EQ(d.path(), name);
    ASSERT_EQ(d.currentEntryPath(), "");
}

TEST(DirentWrapperTests, TraverseSubDirectory)
{
    TemporaryDirectory tempDir = TemporaryDirectory("testdir");
    auto name = std::string(tempDir.name());
    auto subdir =
        buildboxcommontest::TestUtils::createSubDirectory(name, "subdir1");

    // Create subdir2 in tempdir/subdir1/
    auto subdir2 =
        buildboxcommontest::TestUtils::createSubDirectory(subdir, "subdir2");

    // Create a file in subdir1
    TemporaryFile tempFile = TemporaryFile(subdir2.c_str(), "tempfile1");
    auto filename = std::string(tempFile.name());
    auto file_path = subdir2 + "/" + filename;

    // Actually make the file to test stating
    buildboxcommontest::TestUtils::touchFile(file_path.c_str());

    DirentWrapper d(name);
    ASSERT_EQ(d.path(), name);
    ASSERT_EQ(d.currentEntryPath(), subdir);
    auto nextpath = d.currentEntryPath();
    ++d;
    ASSERT_EQ(d.path(), name);
    ASSERT_EQ(d.currentEntryPath(), "");

    // Traverse subdir1, currentEntryPath should be empty.
    DirentWrapper d1(nextpath);
    ASSERT_EQ(d1.path(), subdir);
    ASSERT_EQ(d1.currentEntryPath(), subdir2);
    nextpath = d1.currentEntryPath();
    ++d1;
    ASSERT_EQ(d1.currentEntryPath(), "");

    // Traverse subdir2
    DirentWrapper d2(nextpath);
    ASSERT_EQ(d2.path(), subdir2);
    ASSERT_EQ(d2.currentEntryPath(), filename);
    ASSERT_EQ(d2.currentEntryIsFile(), true);
    ASSERT_EQ(d2.currentEntryIsDirectory(), false);
    ASSERT_EQ(d2.openEntry(O_DIRECTORY), -1);
}

TEST(DirentWrapperTests, TestCorrectSubDirentCreation)
{
    // Test correct DirentWrapper is returned from parent.
    TemporaryDirectory tempDir = TemporaryDirectory("testdir");
    auto name = std::string(tempDir.name());
    auto subdir =
        buildboxcommontest::TestUtils::createSubDirectory(name, "subdir");

    DirentWrapper d(name);
    ASSERT_EQ(d.path(), name);
    ASSERT_EQ(d.currentEntryPath(), subdir);
    ASSERT_EQ(d.currentEntryIsDirectory(), true);

    DirentWrapper n(d.nextDir());
    ASSERT_EQ(n.path(), subdir);
    ASSERT_EQ(n.currentEntryPath(), "");
    ASSERT_EQ(n.currentEntryIsDirectory(), false);
    ASSERT_EQ(n.currentEntryIsFile(), false);
}
