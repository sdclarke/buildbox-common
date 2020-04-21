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

#include "buildboxcommontest_utils.h"
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_temporarydirectory.h>
#include <buildboxcommon_temporaryfile.h>
#include <buildboxcommon_timeutils.h>

#include <gtest/gtest.h>

// To create tempfiles
#include <fstream>
#include <iostream>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace buildboxcommon;

class CreateDirectoryTestFixture : public ::testing::TestWithParam<mode_t> {
  protected:
    CreateDirectoryTestFixture() {}
};

mode_t dirPermissions(const std::string &path)
{
    struct stat stat_buf;
    const int stat_status = stat(path.c_str(), &stat_buf);
    if (stat_status != 0) {
        throw std::system_error(errno, std::system_category(),
                                "Error calling stat(" + path + ")");
    }

    return stat_buf.st_mode & 0777;
}
INSTANTIATE_TEST_CASE_P(CreateDirectoryTests, CreateDirectoryTestFixture,
                        ::testing::Values(0755, 0700));

TEST_P(CreateDirectoryTestFixture, DirectoryTests)
{
    TemporaryDirectory tmpdir;

    const std::string pathStr = std::string(tmpdir.name()) + "/foodir/";
    const char *path = pathStr.c_str();

    ASSERT_FALSE(buildboxcommontest::TestUtils::pathExists(path));
    ASSERT_FALSE(FileUtils::isDirectory(path));
    ASSERT_FALSE(FileUtils::isRegularFile(path));

    const mode_t mode = GetParam();
    FileUtils::createDirectory(path, mode);

    ASSERT_TRUE(buildboxcommontest::TestUtils::pathExists(path));
    ASSERT_TRUE(FileUtils::isDirectory(path));
    ASSERT_FALSE(FileUtils::isRegularFile(path));
    ASSERT_EQ(dirPermissions(path), mode);

    FileUtils::deleteDirectory(path);

    ASSERT_FALSE(buildboxcommontest::TestUtils::pathExists(path));
    ASSERT_FALSE(FileUtils::isDirectory(path));
    ASSERT_FALSE(FileUtils::isRegularFile(path));
}

TEST(CreateDirectoryTestFixture, CreateDirectoryDefaultMode)
{
    const mode_t default_mode = 0755;
    TemporaryDirectory dir;

    const std::string path = std::string(dir.name()) + "/subdir";

    ASSERT_FALSE(FileUtils::isDirectory(path.c_str()));
    FileUtils::createDirectory(path.c_str());
    ASSERT_TRUE(FileUtils::isDirectory(path.c_str()));

    ASSERT_EQ(dirPermissions(path), default_mode);
}

TEST_P(CreateDirectoryTestFixture, CreateDirectorySingleLevel)
{
    TemporaryDirectory dir;

    const std::string path = std::string(dir.name()) + "/subdir";

    ASSERT_FALSE(FileUtils::isDirectory(path.c_str()));

    const mode_t mode = GetParam();
    FileUtils::createDirectory(path.c_str(), mode);

    ASSERT_TRUE(FileUtils::isDirectory(path.c_str()));
    ASSERT_EQ(dirPermissions(path), mode);
}

TEST_P(CreateDirectoryTestFixture, CreateDirectoryPlusItsParents)
{
    const mode_t mode = GetParam();

    TemporaryDirectory dir;
    ASSERT_EQ(chmod(dir.name(), mode), 0);

    const std::string root_directory = dir.name();
    const std::string path = root_directory + "/dir1/dir2/dir3/";

    ASSERT_FALSE(FileUtils::isDirectory(path.c_str()));

    FileUtils::createDirectory(path.c_str(), mode);
    ASSERT_TRUE(FileUtils::isDirectory(path.c_str()));
    ASSERT_EQ(dirPermissions(path), mode);

    // The subdirectories were created with the same mode:
    ASSERT_EQ(dirPermissions(root_directory + "/dir1"), mode);
    ASSERT_EQ(dirPermissions(root_directory + "/dir1/dir2/"), mode);
}

TEST(FileUtilsTest, CreateExistingDirectory)
{
    TemporaryDirectory dir;
    ASSERT_NO_THROW(FileUtils::createDirectory(dir.name()));
}

TEST(FileUtilsTests, IsFile)
{
    TemporaryDirectory tmpdir;
    std::string pathStr = std::string(tmpdir.name()) + "/foo.txt";
    const char *path = pathStr.c_str();

    buildboxcommontest::TestUtils::touchFile(path);

    ASSERT_TRUE(buildboxcommontest::TestUtils::pathExists(path));

    ASSERT_TRUE(FileUtils::isRegularFile(path));
    ASSERT_FALSE(FileUtils::isDirectory(path));
}

TEST(FileUtilsTests, IsFileFD)
{
    TemporaryFile file;

    ASSERT_NE(file.fd(), -1);
    ASSERT_FALSE(FileUtils::isDirectory(file.fd()));
}

TEST(FileUtilsTests, IsNotFileFD)
{
    TemporaryDirectory dir;

    const int dir_fd = open(dir.name(), O_RDONLY);
    ASSERT_NE(dir_fd, -1);

    ASSERT_TRUE(FileUtils::isDirectory(dir_fd));
}

TEST(FileUtilsTests, IsDirectoryBadFdReturnsFalse)
{
    const int bad_fd = -1;
    ASSERT_FALSE(FileUtils::isDirectory(bad_fd));
}

TEST(FileUtilsTests, ExecutableTests)
{
    TemporaryDirectory tmpdir;
    std::string pathStr = std::string(tmpdir.name()) + "/foo.sh";
    const char *path = pathStr.c_str();

    ASSERT_FALSE(buildboxcommontest::TestUtils::pathExists(path));
    ASSERT_FALSE(FileUtils::isExecutable(path));

    buildboxcommontest::TestUtils::touchFile(path);

    ASSERT_TRUE(buildboxcommontest::TestUtils::pathExists(path));
    ASSERT_TRUE(FileUtils::isRegularFile(path));
    ASSERT_FALSE(FileUtils::isExecutable(path));

    FileUtils::makeExecutable(path);

    ASSERT_TRUE(buildboxcommontest::TestUtils::pathExists(path));
    ASSERT_TRUE(FileUtils::isRegularFile(path));
    ASSERT_TRUE(FileUtils::isExecutable(path));
}

TEST(FileUtilsTests, IsSymlink)
{
    TemporaryDirectory dir;
    const auto file_in_dir =
        buildboxcommontest::TestUtils::createFileInDirectory("file1",
                                                             dir.name());
    ASSERT_FALSE(FileUtils::isSymlink(file_in_dir.c_str()));
    const auto symlink_path = dir.strname() + "/symlink";
    symlink(file_in_dir.c_str(), symlink_path.c_str());
    ASSERT_TRUE(FileUtils::isSymlink(symlink_path.c_str()));
}

TEST(FileUtilsTest, DirectoryIsEmptyTest)
{
    TemporaryDirectory dir;

    ASSERT_TRUE(FileUtils::directoryIsEmpty(dir.name()));
}

TEST(FileUtilsTest, DirectoryIsNotEmptyTest)
{
    TemporaryDirectory dir;

    const std::string file_path = std::string(dir.name()) + "/file.txt";

    std::ofstream file(file_path);
    file.close();

    ASSERT_FALSE(FileUtils::directoryIsEmpty(dir.name()));
}

TEST(FileUtilsTest, RemoveSymlinkToDirectory)
{
    TemporaryDirectory dir;
    TemporaryDirectory dir2;
    ASSERT_TRUE(FileUtils::isDirectory(dir.name()));
    ASSERT_TRUE(FileUtils::isDirectory(dir2.name()));

    // Create a symlink to dir from a subdirectory in dir2
    const auto symlink_to_dir =
        std::string(dir2.name()) + "/" + "symlink_to_dir";
    ASSERT_EQ(symlink(dir.name(), symlink_to_dir.c_str()), 0);

    const auto file_in_dir = std::string(dir.name()) + "/" + "file_in_dir.txt";
    buildboxcommontest::TestUtils::touchFile(file_in_dir.c_str());
    ASSERT_TRUE(FileUtils::isRegularFile(file_in_dir.c_str()));

    // Follow the path make sure target is directory.
    ASSERT_TRUE(FileUtils::isDirectory(symlink_to_dir.c_str()));
    ASSERT_FALSE(FileUtils::directoryIsEmpty(dir.name()));

    // Clear dir2
    FileUtils::clearDirectory(dir2.name());
    ASSERT_TRUE(FileUtils::directoryIsEmpty(dir2.name()));

    // Check that dir still exists
    ASSERT_TRUE(FileUtils::isDirectory(dir.name()));
    // Assert file exists in dir
    ASSERT_TRUE(FileUtils::isRegularFile(file_in_dir.c_str()));
    ASSERT_FALSE(FileUtils::directoryIsEmpty(dir.name()));
}

TEST(FileUtilsTests, ClearDirectoryTest)
{
    TemporaryDirectory directory;

    // Populating the directory with a subdirectory and a file:
    const std::string subdirectory_path =
        std::string(directory.name()) + "/subdir";
    FileUtils::createDirectory(subdirectory_path.c_str());

    ASSERT_TRUE(FileUtils::isDirectory(subdirectory_path.c_str()));

    const std::string file_in_subdirectory_path =
        subdirectory_path + "/file1.txt";
    buildboxcommontest::TestUtils::touchFile(
        file_in_subdirectory_path.c_str());

    // Create a symlink in the subdir directory to the test file
    const auto symlink_in_subdir = subdirectory_path + "/file2.txt";
    ASSERT_EQ(0, symlink(file_in_subdirectory_path.c_str(),
                         symlink_in_subdir.c_str()));
    // stat on a symlink will follow the target.
    ASSERT_TRUE(FileUtils::isRegularFile(symlink_in_subdir.c_str()));

    ASSERT_FALSE(FileUtils::directoryIsEmpty(directory.name()));
    FileUtils::clearDirectory(directory.name());

    ASSERT_TRUE(buildboxcommontest::TestUtils::pathExists(directory.name()));
    ASSERT_TRUE(FileUtils::directoryIsEmpty(directory.name()));
}

TEST(NormalizePathTest, AlreadyNormalPaths)
{
    EXPECT_EQ("test.txt", FileUtils::normalizePath("test.txt"));
    EXPECT_EQ("subdir/hello", FileUtils::normalizePath("subdir/hello"));
    EXPECT_EQ("/usr/bin/gcc", FileUtils::normalizePath("/usr/bin/gcc"));
    EXPECT_EQ(".", FileUtils::normalizePath("."));
}

TEST(NormalizePathTest, RemoveEmptySegments)
{
    EXPECT_EQ("subdir/hello", FileUtils::normalizePath("subdir///hello//"));
    EXPECT_EQ("/usr/bin/gcc", FileUtils::normalizePath("/usr/bin/./gcc"));
}

TEST(NormalizePathTest, RemoveUnneededDotDot)
{
    EXPECT_EQ("subdir/hello",
              FileUtils::normalizePath("subdir/subsubdir/../hello"));
    EXPECT_EQ("/usr/bin/gcc",
              FileUtils::normalizePath("/usr/local/lib/../../bin/.//gcc"));
    EXPECT_EQ("/usr/bin/gcc", FileUtils::normalizePath("/../usr/bin/gcc"));
    EXPECT_EQ("/usr/bin/gcc",
              FileUtils::normalizePath("/usr/../../usr/bin/gcc"));
    EXPECT_EQ("/b/c", FileUtils::normalizePath("/a/../b/c/"));
    EXPECT_EQ("b/c", FileUtils::normalizePath("a/../b/c/"));
}

TEST(NormalizePathTest, KeepNeededDotDot)
{
    EXPECT_EQ("../dir/hello", FileUtils::normalizePath("../dir/hello"));
    EXPECT_EQ("../dir/hello",
              FileUtils::normalizePath("subdir/../../dir/hello"));
    EXPECT_EQ("../../dir/hello",
              FileUtils::normalizePath("subdir/../../../dir/hello"));
}

TEST(NormalizePathTest, AlwaysRemoveTrailingSlash)
{
    EXPECT_EQ("/usr/bin", FileUtils::normalizePath("/usr/bin"));
    EXPECT_EQ("/usr/bin", FileUtils::normalizePath("/usr/bin/"));
    EXPECT_EQ(".", FileUtils::normalizePath("./"));
}

TEST(NormalizePathTest, CurrentDirectory)
{
    EXPECT_EQ(".", FileUtils::normalizePath("foo/.."));
    EXPECT_EQ(".", FileUtils::normalizePath("foo/bar/../.."));
    EXPECT_EQ(".", FileUtils::normalizePath("foo/../bar/.."));
}

TEST(MakePathAbsoluteTest, CwdNotAbsoluteThrows)
{
    EXPECT_THROW(FileUtils::FileUtils::makePathAbsolute("a/b/", "a/b"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::FileUtils::makePathAbsolute("/a/b/c", ""),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::FileUtils::makePathAbsolute("", "a/b"),
                 std::runtime_error);
}

TEST(MakePathAbsoluteTest, SimplePaths)
{
    EXPECT_EQ("/a/b/c/d", FileUtils::makePathAbsolute("d", "/a/b/c/"));
    EXPECT_EQ("/a/b/c/d/", FileUtils::makePathAbsolute("d/", "/a/b/c/"));
    EXPECT_EQ("/a/b", FileUtils::makePathAbsolute("..", "/a/b/c/"));
    EXPECT_EQ("/a/b/", FileUtils::makePathAbsolute("../", "/a/b/c/"));
    EXPECT_EQ("/a/b", FileUtils::makePathAbsolute("..", "/a/b/c"));
    EXPECT_EQ("/a/b/", FileUtils::makePathAbsolute("../", "/a/b/c"));

    EXPECT_EQ("/a/b/c", FileUtils::makePathAbsolute(".", "/a/b/c/"));
    EXPECT_EQ("/a/b/c/", FileUtils::makePathAbsolute("./", "/a/b/c/"));
    EXPECT_EQ("/a/b/c", FileUtils::makePathAbsolute(".", "/a/b/c"));
    EXPECT_EQ("/a/b/c/", FileUtils::makePathAbsolute("./", "/a/b/c"));
}

TEST(MakePathAbsoluteTest, MoreComplexPaths)
{
    EXPECT_EQ("/a/b/d", FileUtils::makePathAbsolute("../d", "/a/b/c"));
    EXPECT_EQ("/a/b/d", FileUtils::makePathAbsolute("../d", "/a/b/c/"));
    EXPECT_EQ("/a/b/d/", FileUtils::makePathAbsolute("../d/", "/a/b/c"));
    EXPECT_EQ("/a/b/d/", FileUtils::makePathAbsolute("../d/", "/a/b/c/"));

    EXPECT_EQ("/a/b/d", FileUtils::makePathAbsolute("./.././d", "/a/b/c"));
    EXPECT_EQ("/a/b/d", FileUtils::makePathAbsolute("./.././d", "/a/b/c/"));
    EXPECT_EQ("/a/b/d/", FileUtils::makePathAbsolute("./.././d/", "/a/b/c"));
    EXPECT_EQ("/a/b/d/", FileUtils::makePathAbsolute("./.././d/", "/a/b/c/"));
}

TEST(MakePathAbsoluteTest, AbsolutePaths)
{
    EXPECT_EQ("/x/y/z", FileUtils::makePathAbsolute("/x/y/z", "/a/b/c"));

    // verify that the path still gets normalized
    EXPECT_EQ("/x/y/m",
              FileUtils::makePathAbsolute("/x/y/z/.././m", "/a/b/c"));
}

TEST(JoinPathSegmentsTest, JoinPaths)
{
    // Relative path first segment
    EXPECT_EQ("/b", FileUtils::joinPathSegments("a", "/b"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("a/", "/b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "b"));

    EXPECT_EQ("/b", FileUtils::joinPathSegments("a", "/b/"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("a/", "/b/"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "b/"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "b/"));

    EXPECT_EQ("/c", FileUtils::joinPathSegments("a/b", "/c"));
    EXPECT_EQ("/c", FileUtils::joinPathSegments("a/b/", "/c"));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/b", "c"));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/b/", "c"));

    EXPECT_EQ("/b/c", FileUtils::joinPathSegments("a", "/b/c"));
    EXPECT_EQ("/b/c", FileUtils::joinPathSegments("a/", "/b/c"));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a", "b/c"));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/", "b/c"));

    // Absolute path first segment
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a", "b"));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a/", "b"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("/a", "/b"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("/a/", "/b"));

    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a", "b/"));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a/", "b/"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("/a", "/b/"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("/a/", "/b/"));

    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/b", "c"));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/b/", "c"));
    EXPECT_EQ("/c", FileUtils::joinPathSegments("/a/b", "/c"));
    EXPECT_EQ("/c", FileUtils::joinPathSegments("/a/b/", "/c"));

    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a", "b/c"));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/", "b/c"));
    ;
    EXPECT_EQ("/b/c", FileUtils::joinPathSegments("/a/", "/b/c"));

    // paths containing '.'
    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a", "."));
    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a/", "."));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a", "."));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a/", "."));

    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a", "./"));
    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a/", "./"));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a", "./"));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a/", "./"));

    EXPECT_EQ("/b", FileUtils::joinPathSegments("/./a/.", "/./b"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("/a/.", "/./b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("./a", "./b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/./", "./b"));

    EXPECT_EQ("/b", FileUtils::joinPathSegments("a", "/./b"));
    EXPECT_EQ("/b", FileUtils::joinPathSegments("a/", "/./b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "./b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "./b"));

    EXPECT_EQ("/b", FileUtils::joinPathSegments("a/.", "/./b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("./a/", "./b"));

    // paths containing '..' (Escapes allowed)
    EXPECT_EQ("/a/c/d/e/f", FileUtils::joinPathSegments("/a/b/../c", "d/e/f"));
    EXPECT_EQ("/b/c", FileUtils::joinPathSegments("/a", "../b/c"));
    EXPECT_EQ("/c", FileUtils::joinPathSegments("/a", "/b/../c"));
}

TEST(JoinPathSegmentsTest, JoinPathsForceSecondSegmentRelative)
{
    // Relative path first segment
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "/b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "/b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "/b/", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "/b/", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "b/", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "b/", true));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/b", "/c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/b/", "/c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/b", "c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/b/", "c", true));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a", "/b/c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/", "/b/c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a", "b/c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegments("a/", "b/c", true));

    // Absolute path first segment
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a", "b", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a/", "b", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a", "/b", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a/", "/b", true));

    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a", "b/", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a/", "b/", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a", "/b/", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a/", "/b/", true));

    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/b", "c", true));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/b/", "c", true));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/b", "/c", true));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/b/", "/c", true));

    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a", "b/c", true));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/", "b/c", true));
    ;
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegments("/a/", "/b/c", true));

    // paths containing '.'
    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a", ".", true));
    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a/", ".", true));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a", ".", true));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a/", ".", true));

    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a", "./", true));
    EXPECT_EQ("/a", FileUtils::joinPathSegments("/a/", "./", true));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a", "./", true));
    EXPECT_EQ("a", FileUtils::joinPathSegments("a/", "./", true));

    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/./a/.", "/./b", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegments("/a/.", "/./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("./a", "./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/./", "./b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "/./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "/./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a", "./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/", "./b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegments("a/.", "/./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegments("./a/", "./b", true));

    // paths containing '..' (Escapes allowed)
    EXPECT_EQ("/a/c/d/e/f",
              FileUtils::joinPathSegments("/a/b/../c", "d/e/f", true));
    EXPECT_EQ("/b/c", FileUtils::joinPathSegments("/a", "../b/c", true));
    EXPECT_EQ("/a/c", FileUtils::joinPathSegments("/a", "/b/../c", true));
}

TEST(JoinPathSegmentsTestInvalidArgs, JoinPaths)
{
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegments("", ""),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegments("a/b", ""),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegments("", "a/b"),
                 std::runtime_error);
}

TEST(JoinPathSegmentsNoEscapeTest, JoinPathsNoEscape)
{
    // Relative path first segment
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "b"));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "b/"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "b/"));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a/b", "c"));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a/b/", "c"));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a", "b/c"));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a/", "b/c"));

    // Absolute path first segment
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a", "b"));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a/", "b"));

    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a", "b/"));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a/", "b/"));

    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegmentsNoEscape("/a/b", "c"));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegmentsNoEscape("/a/b/", "c"));

    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegmentsNoEscape("/a", "b/c"));
    EXPECT_EQ("/a/b/c", FileUtils::joinPathSegmentsNoEscape("/a/", "b/c"));

    // paths containing '.'
    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a", "."));
    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a/", "."));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a", "."));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a/", "."));

    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a", "./"));
    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a/", "./"));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a", "./"));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a/", "./"));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("./a", "./b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/./", "./b"));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "./b"));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "./b"));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("./a/", "./b"));

    // paths containing '..' (Escapes outside first dir NOT allowed)
    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a", "b/c/../../"));
    ;
    EXPECT_EQ("/a/b/f",
              FileUtils::joinPathSegmentsNoEscape("/a", "b/c/../d/../e/../f"));
    EXPECT_EQ("/a/c/d/e/f",
              FileUtils::joinPathSegmentsNoEscape("/a/b/../c", "d/e/f"));
    EXPECT_EQ("/c/d/e",
              FileUtils::joinPathSegmentsNoEscape("/a/../", "c/d/e"));
}

TEST(JoinPathSegmentsNoEscapeTest,
     JoinPathsNoEscapeForceRelativePathWithinBaseDir)
{
    // Relative path first segment
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "b/", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "b/", true));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a/b", "c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a/b/", "c", true));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a", "b/c", true));
    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a/", "b/c", true));

    // Absolute path first segment
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a", "b", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a/", "b", true));

    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a", "b/", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a/", "b/", true));

    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a/b", "c", true));
    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a/b/", "c", true));

    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a", "b/c", true));
    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a/", "b/c", true));

    // paths containing '.'
    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a", ".", true));
    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a/", ".", true));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a", ".", true));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a/", ".", true));

    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a", "./", true));
    EXPECT_EQ("/a", FileUtils::joinPathSegmentsNoEscape("/a/", "./", true));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a", "./", true));
    EXPECT_EQ("a", FileUtils::joinPathSegmentsNoEscape("a/", "./", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("./a", "./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/./", "./b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "./b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("./a/", "./b", true));

    // paths containing '..' (Escapes outside first dir NOT allowed)
    EXPECT_EQ("/a",
              FileUtils::joinPathSegmentsNoEscape("/a", "b/c/../../", true));
    ;
    EXPECT_EQ("/a/b/f", FileUtils::joinPathSegmentsNoEscape(
                            "/a", "b/c/../d/../e/../f", true));
    EXPECT_EQ("/a/c/d/e/f",
              FileUtils::joinPathSegmentsNoEscape("/a/b/../c", "d/e/f", true));
    EXPECT_EQ("/c/d/e",
              FileUtils::joinPathSegmentsNoEscape("/a/../", "c/d/e", true));

    // Not escaping due to forceRelativePathWithinBaseDir
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "/b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "/b/", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "/b/", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a/", "/b", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a/", "/b/", true));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a/b", "/c", true));
    EXPECT_EQ("a/b/c",
              FileUtils::joinPathSegmentsNoEscape("a/b/", "/c", true));

    EXPECT_EQ("a/b/c", FileUtils::joinPathSegmentsNoEscape("a", "/b/c", true));
    EXPECT_EQ("a/b/c",
              FileUtils::joinPathSegmentsNoEscape("a/", "/b/c", true));

    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a", "/b", true));
    EXPECT_EQ("/a/b", FileUtils::joinPathSegmentsNoEscape("/a/", "/b", true));

    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a/b", "/c", true));
    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a/b/", "/c", true));

    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a", "/b/c", true));
    EXPECT_EQ("/a/b/c",
              FileUtils::joinPathSegmentsNoEscape("/a/", "/b/c", true));

    EXPECT_EQ("/a/b",
              FileUtils::joinPathSegmentsNoEscape("/./a/.", "/./b", true));
    EXPECT_EQ("/a/b",
              FileUtils::joinPathSegmentsNoEscape("/a/.", "/./b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a", "/./b", true));
    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/", "/./b", true));

    EXPECT_EQ("a/b", FileUtils::joinPathSegmentsNoEscape("a/.", "/./b", true));
    EXPECT_EQ("/a/c",
              FileUtils::joinPathSegmentsNoEscape("/a", "/b/../c", true));
}

TEST(JoinPathSegmentsNoEscapeTestEscapesThrow, JoinPathsNoEscape)
{
    // Base dir escapes
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("a/../..", "/b"),
        std::runtime_error);
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("a/../..", "b"),
        std::runtime_error);
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("a/b/../../..", "/b"),
        std::runtime_error);
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("a/b/../../..", "b"),
        std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/../", "c/d/e"),
                 std::runtime_error);

    // Path within basedir escapes
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("/a", "../b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("/a", "/../b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("/a", "/b"),
                 std::runtime_error);
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("/a", "b/c/../../../"),
        std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("/a/", "../b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("/a/", "/b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("/a/b/c/",
                                                                "d/../../e/f"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("a", "/b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a", "../b/c"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a", "/b/../c"),
                 std::runtime_error);

    // Escaping due to absolue paths
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a", "/b/"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/", "/b/"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/b", "/c"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/b/", "/c"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a", "/b/c"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/", "/b/c"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a", "/b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a/", "/b"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a/b", "/c"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a/b/", "/c"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a", "/b/c"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a/", "/b/c"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/./a/.", "/./b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a/.", "/./b"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a", "/./b"),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/", "/./b"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/.", "/./b"),
                 std::runtime_error);
}

TEST(JoinPathSegmentsNoEscapeTestForceRelativePathWithinBaseDirEscapesThrow,
     JoinPathsNoEscape)
{
    // Base dir escapes
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("a/../..", "/b", true),
        std::runtime_error);
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("a/../..", "b", true),
        std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("a/b/../../..",
                                                                "/b", true),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape("a/b/../../..",
                                                                "b", true),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("a/../", "c/d/e", true),
                 std::runtime_error);

    // Path within basedir escapes
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("/a", "../b", true),
        std::runtime_error);

    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape(
                     "/a", "b/c/../../../", true),
                 std::runtime_error);
    EXPECT_THROW(
        FileUtils::FileUtils::joinPathSegmentsNoEscape("/a/", "../b", true),
        std::runtime_error);
    EXPECT_THROW(FileUtils::FileUtils::joinPathSegmentsNoEscape(
                     "/a/b/c/", "d/../../e/f", true),
                 std::runtime_error);
    EXPECT_THROW(FileUtils::joinPathSegmentsNoEscape("/a", "../b/c", true),
                 std::runtime_error);
}

TEST(MakePathRelativeTest, ReturnNonAbsolutePathsUnmodified)
{
    EXPECT_EQ("", FileUtils::makePathRelative("", "/some/working/directory"));
    EXPECT_EQ("../a/relative/path",
              FileUtils::makePathRelative("../a/relative/path",
                                          "/some/working/directory"));
    EXPECT_EQ("test",
              FileUtils::makePathRelative("test", "/some/working/directory"));
    EXPECT_EQ("test/../path", FileUtils::makePathRelative(
                                  "test/../path", "/some/working/directory"));
    EXPECT_EQ("test/long/path",
              FileUtils::makePathRelative("test/long/path",
                                          "/some/working/directory"));
    EXPECT_EQ("some/path", FileUtils::makePathRelative(
                               "some/path", "/some/working/directory"));
    EXPECT_EQ("./some/path", FileUtils::makePathRelative(
                                 "./some/path", "/some/working/directory"));
    EXPECT_EQ("some/long/path/..",
              FileUtils::makePathRelative("some/long/path/..",
                                          "/some/working/directory"));
}

TEST(MakePathRelativeTest, DoNothingIfWorkingDirectoryNull)
{
    EXPECT_EQ("/test/directory/",
              FileUtils::makePathRelative("/test/directory/", ""));
    EXPECT_EQ("/test", FileUtils::makePathRelative("/test", ""));
}

TEST(MakePathRelativeTest, WorkingDirectoryIsPathPrefix)
{
    EXPECT_EQ("some/test/path",
              FileUtils::makePathRelative("/some/test/path", "/"));

    EXPECT_EQ("test/path",
              FileUtils::makePathRelative("/some/test/path", "/some"));
    EXPECT_EQ("test/path",
              FileUtils::makePathRelative("/some/test/path", "/some/"));

    EXPECT_EQ("path",
              FileUtils::makePathRelative("/some/test/path", "/some/test"));
    EXPECT_EQ("path",
              FileUtils::makePathRelative("/some/test/path", "/some/test/"));

    EXPECT_EQ("path/",
              FileUtils::makePathRelative("/some/test/path/", "/some/test"));
    EXPECT_EQ("path/",
              FileUtils::makePathRelative("/some/test/path/", "/some/test/"));
}

TEST(MakePathRelativeTest, PathEqualsWorkingDirectory)
{
    EXPECT_EQ(".", FileUtils::makePathRelative("/some/test/path",
                                               "/some/test/path"));
    EXPECT_EQ(".", FileUtils::makePathRelative("/some/test/path",
                                               "/some/test/path/"));
    EXPECT_EQ("./", FileUtils::makePathRelative("/some/test/path/",
                                                "/some/test/path"));
    EXPECT_EQ("./", FileUtils::makePathRelative("/some/test/path/",
                                                "/some/test/path/"));
}

TEST(MakePathRelativeTest, PathAlmostEqualsWorkingDirectory)
{
    EXPECT_EQ("../tests",
              FileUtils::makePathRelative("/some/tests", "/some/test"));
    EXPECT_EQ("../tests",
              FileUtils::makePathRelative("/some/tests", "/some/test/"));
    EXPECT_EQ("../tests/",
              FileUtils::makePathRelative("/some/tests/", "/some/test"));
    EXPECT_EQ("../tests/",
              FileUtils::makePathRelative("/some/tests/", "/some/test/"));
}

TEST(MakePathRelativeTest, PathIsParentOfWorkingDirectory)
{
    EXPECT_EQ("..", FileUtils::makePathRelative("/a/b/c", "/a/b/c/d"));
    EXPECT_EQ("..", FileUtils::makePathRelative("/a/b/c", "/a/b/c/d/"));
    EXPECT_EQ("../", FileUtils::makePathRelative("/a/b/c/", "/a/b/c/d"));
    EXPECT_EQ("../", FileUtils::makePathRelative("/a/b/c/", "/a/b/c/d/"));

    EXPECT_EQ("../../..", FileUtils::makePathRelative("/a", "/a/b/c/d"));
    EXPECT_EQ("../../..", FileUtils::makePathRelative("/a", "/a/b/c/d/"));
    EXPECT_EQ("../../../", FileUtils::makePathRelative("/a/", "/a/b/c/d"));
    EXPECT_EQ("../../../", FileUtils::makePathRelative("/a/", "/a/b/c/d/"));
}

TEST(MakePathRelativeTest, PathAdjacentToWorkingDirectory)
{
    EXPECT_EQ("../e", FileUtils::makePathRelative("/a/b/c/e", "/a/b/c/d"));
    EXPECT_EQ("../e", FileUtils::makePathRelative("/a/b/c/e", "/a/b/c/d/"));
    EXPECT_EQ("../e/", FileUtils::makePathRelative("/a/b/c/e/", "/a/b/c/d"));
    EXPECT_EQ("../e/", FileUtils::makePathRelative("/a/b/c/e/", "/a/b/c/d/"));

    EXPECT_EQ("../e/f/g",
              FileUtils::makePathRelative("/a/b/c/e/f/g", "/a/b/c/d"));
    EXPECT_EQ("../e/f/g",
              FileUtils::makePathRelative("/a/b/c/e/f/g", "/a/b/c/d/"));
    EXPECT_EQ("../e/f/g/",
              FileUtils::makePathRelative("/a/b/c/e/f/g/", "/a/b/c/d"));
    EXPECT_EQ("../e/f/g/",
              FileUtils::makePathRelative("/a/b/c/e/f/g/", "/a/b/c/d/"));

    EXPECT_EQ("../../e/f/g",
              FileUtils::makePathRelative("/a/b/e/f/g", "/a/b/c/d"));
    EXPECT_EQ("../../e/f/g",
              FileUtils::makePathRelative("/a/b/e/f/g", "/a/b/c/d/"));
    EXPECT_EQ("../../e/f/g/",
              FileUtils::makePathRelative("/a/b/e/f/g/", "/a/b/c/d"));
    EXPECT_EQ("../../e/f/g/",
              FileUtils::makePathRelative("/a/b/e/f/g/", "/a/b/c/d/"));
}

TEST(FileUtilsTests, WriteFileAtomically)
{
    TemporaryDirectory output_directory;

    const std::string output_path =
        std::string(output_directory.name()) + "/data.txt";

    ASSERT_FALSE(FileUtils::isRegularFile(output_path.c_str()));

    std::vector<char> raw_data = {'H', 'e', 'l', 'l', 'o',  '\0', 'W',
                                  'o', 'r', 'l', 'd', '\0', '!'};
    const std::string data_string(raw_data.cbegin(), raw_data.cend());

    ASSERT_EQ(FileUtils::writeFileAtomically(output_path, data_string), 0);

    // Data is correct:
    std::ifstream file(output_path, std::ifstream::binary);
    std::stringstream read_data;
    read_data << file.rdbuf();

    ASSERT_EQ(read_data.str(), data_string);

    // Default mode is 0600:
    struct stat stat_buf;
    const int stat_status = stat(output_path.c_str(), &stat_buf);
    ASSERT_EQ(stat_status, 0);

    ASSERT_TRUE(S_ISREG(stat_buf.st_mode));

    const auto file_permissions = stat_buf.st_mode & 0777;
    ASSERT_EQ(file_permissions, 0600);
}

TEST(FileUtilsTests, WriteFileAtomicallyReturnsLinkResult)
{
    TemporaryDirectory output_directory;

    const std::string output_path =
        std::string(output_directory.name()) + "/output.txt";

    ASSERT_FALSE(FileUtils::isRegularFile(output_path.c_str()));

    ASSERT_EQ(FileUtils::writeFileAtomically(output_path, ""), 0);
    ASSERT_EQ(FileUtils::writeFileAtomically(output_path, ""), EEXIST);
}

TEST(FileUtilsTests, WriteFileAtomicallyPermissions)
{
    TemporaryDirectory output_directory;

    const std::string output_path =
        std::string(output_directory.name()) + "/executable.sh";

    ASSERT_FALSE(FileUtils::isRegularFile(output_path.c_str()));

    const std::string data = "#!/bin/bash";
    ASSERT_EQ(FileUtils::writeFileAtomically(output_path, data, 0740), 0);

    struct stat stat_buf;
    const int stat_status = stat(output_path.c_str(), &stat_buf);
    ASSERT_EQ(stat_status, 0);

    ASSERT_TRUE(S_ISREG(stat_buf.st_mode));

    const auto file_permissions = stat_buf.st_mode & 0777;
    ASSERT_EQ(file_permissions, 0740);
}

TEST(FileUtilsTests, WriteFileAtomicallyTemporaryDirectory)
{
    TemporaryDirectory output_directory;
    TemporaryDirectory intermediate_directory;

    const std::string output_path =
        std::string(output_directory.name()) + "/test.txt";

    ASSERT_FALSE(FileUtils::isRegularFile(output_path.c_str()));

    const auto data = "some data...";
    FileUtils::writeFileAtomically(output_path, data, 0600,
                                   intermediate_directory.name());

    ASSERT_TRUE(FileUtils::isRegularFile(output_path.c_str()));

    // Data is correct:
    std::ifstream file(output_path, std::ifstream::binary);
    std::stringstream read_data;
    read_data << file.rdbuf();

    ASSERT_EQ(read_data.str(), data);
}

TEST(FileUtilsTests, WriteFileAtomicallyIntermediateFileIsDeleted)
{
    TemporaryDirectory test_directory;
    const std::string test_directory_path(test_directory.name());
    const auto output_path = test_directory_path + "/out.txt";

    const auto intermediate_directory = test_directory_path + "/intermediate";
    FileUtils::createDirectory(intermediate_directory.c_str());

    FileUtils::writeFileAtomically(output_path, "data: 12345", 0600,
                                   intermediate_directory);
    ASSERT_TRUE(FileUtils::isRegularFile(output_path.c_str()));

    // The intermediate file was deleted:
    ASSERT_TRUE(FileUtils::directoryIsEmpty(intermediate_directory.c_str()));
}

TEST(FileUtilsTests, PathBasenameTests)
{
    EXPECT_EQ("hello", FileUtils::pathBasename("a/b/hello"));
    EXPECT_EQ("hello.txt", FileUtils::pathBasename("a/b/hello.txt"));
    EXPECT_EQ("hello", FileUtils::pathBasename("//hello/a/b/hello"));
    EXPECT_EQ("hello", FileUtils::pathBasename("a/b/../../hello"));
    EXPECT_EQ("hello", FileUtils::pathBasename("a/b/hello/"));
    EXPECT_EQ("hello", FileUtils::pathBasename("/a/hello/"));
    EXPECT_EQ("", FileUtils::pathBasename("/"));
}

TEST(FileUtilsTests, GetFileMtime)
{
    TemporaryDirectory tmpdir;
    std::string pathStr = std::string(tmpdir.name()) + "/foo.sh";
    const char *path = pathStr.c_str();

    // this should be fast enough
    ASSERT_FALSE(buildboxcommontest::TestUtils::pathExists(path));
    buildboxcommontest::TestUtils::touchFile(path);
    std::chrono::system_clock::time_point now =
        std::chrono::system_clock::now();
    ASSERT_TRUE(buildboxcommontest::TestUtils::pathExists(path));

    std::chrono::system_clock::time_point mtime =
        FileUtils::getFileMtime(path);
    std::chrono::seconds timediff =
        std::chrono::duration_cast<std::chrono::seconds>(now - mtime);
    ASSERT_EQ(timediff.count(), 0);

    const int fd = open(path, O_RDONLY);
    mtime = FileUtils::getFileMtime(fd);
    timediff = std::chrono::duration_cast<std::chrono::seconds>(now - mtime);
    ASSERT_EQ(timediff.count(), 0);
}

TEST(FileUtilsTests, ModifyFileTimestamp)
{
    // Depends upon ParseTimePoint, ParseTimeStamp and GetFileMtime
    TemporaryDirectory tmpdir;
    std::string pathStr = std::string(tmpdir.name()) + "/foo.sh";
    const char *path = pathStr.c_str();

    ASSERT_FALSE(buildboxcommontest::TestUtils::pathExists(path));
    buildboxcommontest::TestUtils::touchFile(path);
    ASSERT_TRUE(buildboxcommontest::TestUtils::pathExists(path));

    // try e2e test of file timestamps
    // get the original time
    auto orig_time = FileUtils::getFileMtime(path);
    auto orig_count = std::chrono::duration_cast<std::chrono::microseconds>(
                          orig_time.time_since_epoch())
                          .count();
    // get a new time to set and sanity check it
    const long int exp_count = 1325586092000000;
    ASSERT_NE(exp_count, orig_count);

    const std::string new_stamp = "2012-01-03T10:21:32.000000Z";
    auto new_time = TimeUtils::parse_timestamp(new_stamp);
    auto new_count = std::chrono::duration_cast<std::chrono::microseconds>(
                         new_time.time_since_epoch())
                         .count();
    ASSERT_EQ(exp_count, new_count);

    // try to set file mtime
    FileUtils::setFileMtime(path, new_time);
    // check the file mtime
    auto mtime = FileUtils::getFileMtime(path);
    auto count = std::chrono::duration_cast<std::chrono::microseconds>(
                     mtime.time_since_epoch())
                     .count();
    ASSERT_EQ(count, new_count);

    // and change it back
    const int fd = open(path, O_RDWR);
    FileUtils::setFileMtime(fd, orig_time);
    mtime = FileUtils::getFileMtime(fd);
    count = std::chrono::duration_cast<std::chrono::microseconds>(
                mtime.time_since_epoch())
                .count();
    ASSERT_EQ(count, orig_count);
}

TEST(FileUtilsTests, CopyFile)
{
    TemporaryDirectory output_directory;

    const std::string output_path =
        std::string(output_directory.name()) + "/executable.sh";

    ASSERT_FALSE(FileUtils::isRegularFile(output_path.c_str()));

    const std::string data = "#!/bin/bash";
    ASSERT_EQ(FileUtils::writeFileAtomically(output_path, data, 0744,
                                             output_directory.name()),
              0);
    ASSERT_TRUE(
        buildboxcommontest::TestUtils::pathExists(output_path.c_str()));

    // Try to copy this
    const std::string copy_path =
        std::string(output_directory.name()) + "/copy.sh";
    ASSERT_FALSE(buildboxcommontest::TestUtils::pathExists(copy_path.c_str()));
    FileUtils::copyFile(output_path.c_str(), copy_path.c_str());

    // Data is correct:
    std::ifstream file(copy_path, std::ifstream::binary);
    std::stringstream read_data;
    read_data << file.rdbuf();

    ASSERT_EQ(read_data.str(), data);

    // It is a regular file
    struct stat stat_buf;
    const int stat_status = stat(copy_path.c_str(), &stat_buf);
    ASSERT_EQ(stat_status, 0);

    ASSERT_TRUE(S_ISREG(stat_buf.st_mode));

    // It have the correct permissions:
    const auto file_permissions = stat_buf.st_mode & 0777;
    ASSERT_EQ(file_permissions, 0744);
}
