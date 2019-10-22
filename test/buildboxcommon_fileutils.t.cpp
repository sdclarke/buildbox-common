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
#include <buildboxcommon_temporaryfile.h>
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

bool path_exists(const char *path)
{
    struct stat statResult;
    return stat(path, &statResult) == 0;
}

void touch_file(const char *path)
{
    std::fstream fs;
    fs.open(path, std::ios::out);
    fs.close();
}

TEST(FileUtilsTests, DirectoryTests)
{
    TemporaryDirectory tmpdir;
    std::string pathStr = std::string(tmpdir.name()) + "/foodir/";
    const char *path = pathStr.c_str();

    ASSERT_FALSE(path_exists(path));
    ASSERT_FALSE(FileUtils::is_directory(path));
    ASSERT_FALSE(FileUtils::is_regular_file(path));

    FileUtils::create_directory(path);

    ASSERT_TRUE(path_exists(path));
    ASSERT_TRUE(FileUtils::is_directory(path));
    ASSERT_FALSE(FileUtils::is_regular_file(path));

    FileUtils::delete_directory(path);

    ASSERT_FALSE(path_exists(path));
    ASSERT_FALSE(FileUtils::is_directory(path));
    ASSERT_FALSE(FileUtils::is_regular_file(path));
}

TEST(FileUtilsTests, IsFile)
{
    TemporaryDirectory tmpdir;
    std::string pathStr = std::string(tmpdir.name()) + "/foo.txt";
    const char *path = pathStr.c_str();

    touch_file(path);

    ASSERT_TRUE(path_exists(path));

    ASSERT_TRUE(FileUtils::is_regular_file(path));
    ASSERT_FALSE(FileUtils::is_directory(path));
}

TEST(FileUtilsTests, IsFileFD)
{
    TemporaryFile file;

    ASSERT_NE(file.fd(), -1);
    ASSERT_FALSE(FileUtils::is_directory(file.fd()));
}

TEST(FileUtilsTests, IsNotFileFD)
{
    TemporaryDirectory dir;

    const int dir_fd = open(dir.name(), O_RDONLY);
    ASSERT_NE(dir_fd, -1);

    ASSERT_TRUE(FileUtils::is_directory(dir_fd));
}

TEST(FileUtilsTests, IsDirectoryBadFdReturnsFalse)
{
    const int bad_fd = -1;
    ASSERT_FALSE(FileUtils::is_directory(bad_fd));
}

TEST(FileUtilsTests, ExecutableTests)
{
    TemporaryDirectory tmpdir;
    std::string pathStr = std::string(tmpdir.name()) + "/foo.sh";
    const char *path = pathStr.c_str();

    ASSERT_FALSE(path_exists(path));
    ASSERT_FALSE(FileUtils::is_executable(path));

    touch_file(path);

    ASSERT_TRUE(path_exists(path));
    ASSERT_TRUE(FileUtils::is_regular_file(path));
    ASSERT_FALSE(FileUtils::is_executable(path));

    FileUtils::make_executable(path);

    ASSERT_TRUE(path_exists(path));
    ASSERT_TRUE(FileUtils::is_regular_file(path));
    ASSERT_TRUE(FileUtils::is_executable(path));
}

TEST(FileUtilsTest, DirectoryIsEmptyTest)
{
    TemporaryDirectory dir;

    ASSERT_TRUE(FileUtils::directory_is_empty(dir.name()));
}

TEST(FileUtilsTest, DirectoryIsNotEmptyTest)
{
    TemporaryDirectory dir;

    const std::string file_path = std::string(dir.name()) + "/file.txt";

    std::ofstream file(file_path);
    file.close();

    ASSERT_FALSE(FileUtils::directory_is_empty(dir.name()));
}

TEST(FileUtilsTests, ClearDirectoryTest)
{
    TemporaryDirectory directory;

    // Populating the directory with a subdirectory and a file:
    const std::string subdirectory_path =
        std::string(directory.name()) + "/subdir";
    FileUtils::create_directory(subdirectory_path.c_str());

    ASSERT_TRUE(FileUtils::is_directory(subdirectory_path.c_str()));

    const std::string file_in_subdirectory_path =
        subdirectory_path + "/file1.txt";
    std::ofstream file(file_in_subdirectory_path);
    file.close();

    ASSERT_FALSE(FileUtils::directory_is_empty(directory.name()));

    FileUtils::clear_directory(directory.name());

    ASSERT_TRUE(path_exists(directory.name()));
    ASSERT_TRUE(FileUtils::directory_is_empty(directory.name()));
}

TEST(NormalizePathTest, AlreadyNormalPaths)
{
    EXPECT_EQ("test.txt", FileUtils::normalize_path("test.txt"));
    EXPECT_EQ("subdir/hello", FileUtils::normalize_path("subdir/hello"));
    EXPECT_EQ("/usr/bin/gcc", FileUtils::normalize_path("/usr/bin/gcc"));
}

TEST(NormalizePathTest, RemoveEmptySegments)
{
    EXPECT_EQ("subdir/hello", FileUtils::normalize_path("subdir///hello//"));
    EXPECT_EQ("/usr/bin/gcc", FileUtils::normalize_path("/usr/bin/./gcc"));
}

TEST(NormalizePathTest, RemoveUnneededDotDot)
{
    EXPECT_EQ("subdir/hello",
              FileUtils::normalize_path("subdir/subsubdir/../hello"));
    EXPECT_EQ("/usr/bin/gcc",
              FileUtils::normalize_path("/usr/local/lib/../../bin/.//gcc"));
}

TEST(NormalizePathTest, KeepNeededDotDot)
{
    EXPECT_EQ("../dir/hello", FileUtils::normalize_path("../dir/hello"));
    EXPECT_EQ("../dir/hello",
              FileUtils::normalize_path("subdir/../../dir/hello"));
    EXPECT_EQ("../../dir/hello",
              FileUtils::normalize_path("subdir/../../../dir/hello"));
}

TEST(NormalizePathTest, AlwaysRemoveTrailingSlash)
{
    EXPECT_EQ("/usr/bin", FileUtils::normalize_path("/usr/bin"));
    EXPECT_EQ("/usr/bin", FileUtils::normalize_path("/usr/bin/"));
}

TEST(MakePathAbsoluteTest, CwdNotAbsoluteThrows)
{
    EXPECT_THROW(FileUtils::FileUtils::make_path_absolute("a/b/", "a/b"),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::FileUtils::make_path_absolute("/a/b/c", ""),
                 std::runtime_error);

    EXPECT_THROW(FileUtils::FileUtils::make_path_absolute("", "a/b"),
                 std::runtime_error);
}

TEST(MakePathAbsoluteTest, SimplePaths)
{
    EXPECT_EQ("/a/b/c/d", FileUtils::make_path_absolute("d", "/a/b/c/"));
    EXPECT_EQ("/a/b/c/d/", FileUtils::make_path_absolute("d/", "/a/b/c/"));
    EXPECT_EQ("/a/b", FileUtils::make_path_absolute("..", "/a/b/c/"));
    EXPECT_EQ("/a/b/", FileUtils::make_path_absolute("../", "/a/b/c/"));
    EXPECT_EQ("/a/b", FileUtils::make_path_absolute("..", "/a/b/c"));
    EXPECT_EQ("/a/b/", FileUtils::make_path_absolute("../", "/a/b/c"));

    EXPECT_EQ("/a/b/c", FileUtils::make_path_absolute(".", "/a/b/c/"));
    EXPECT_EQ("/a/b/c/", FileUtils::make_path_absolute("./", "/a/b/c/"));
    EXPECT_EQ("/a/b/c", FileUtils::make_path_absolute(".", "/a/b/c"));
    EXPECT_EQ("/a/b/c/", FileUtils::make_path_absolute("./", "/a/b/c"));
}

TEST(MakePathAbsoluteTest, MoreComplexPaths)
{
    EXPECT_EQ("/a/b/d", FileUtils::make_path_absolute("../d", "/a/b/c"));
    EXPECT_EQ("/a/b/d", FileUtils::make_path_absolute("../d", "/a/b/c/"));
    EXPECT_EQ("/a/b/d/", FileUtils::make_path_absolute("../d/", "/a/b/c"));
    EXPECT_EQ("/a/b/d/", FileUtils::make_path_absolute("../d/", "/a/b/c/"));

    EXPECT_EQ("/a/b/d", FileUtils::make_path_absolute("./.././d", "/a/b/c"));
    EXPECT_EQ("/a/b/d", FileUtils::make_path_absolute("./.././d", "/a/b/c/"));
    EXPECT_EQ("/a/b/d/", FileUtils::make_path_absolute("./.././d/", "/a/b/c"));
    EXPECT_EQ("/a/b/d/",
              FileUtils::make_path_absolute("./.././d/", "/a/b/c/"));
}

TEST(FileUtilsTests, WriteFileAtomically)
{
    TemporaryDirectory output_directory;

    const std::string output_path =
        std::string(output_directory.name()) + "/data.txt";

    ASSERT_FALSE(FileUtils::is_regular_file(output_path.c_str()));

    std::vector<char> raw_data = {'H', 'e', 'l', 'l', 'o',  '\0', 'W',
                                  'o', 'r', 'l', 'd', '\0', '!'};
    const std::string data_string(raw_data.cbegin(), raw_data.cend());

    ASSERT_EQ(FileUtils::write_file_atomically(output_path, data_string), 0);

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

    ASSERT_FALSE(FileUtils::is_regular_file(output_path.c_str()));

    ASSERT_EQ(FileUtils::write_file_atomically(output_path, ""), 0);
    ASSERT_EQ(FileUtils::write_file_atomically(output_path, ""), EEXIST);
}

TEST(FileUtilsTests, WriteFileAtomicallyPermissions)
{
    TemporaryDirectory output_directory;

    const std::string output_path =
        std::string(output_directory.name()) + "/executable.sh";

    ASSERT_FALSE(FileUtils::is_regular_file(output_path.c_str()));

    const std::string data = "#!/bin/bash";
    ASSERT_EQ(FileUtils::write_file_atomically(output_path, data, 0740), 0);

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

    ASSERT_FALSE(FileUtils::is_regular_file(output_path.c_str()));

    const auto data = "some data...";
    FileUtils::write_file_atomically(output_path, data, 0600,
                                     intermediate_directory.name());

    ASSERT_TRUE(FileUtils::is_regular_file(output_path.c_str()));

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
    FileUtils::create_directory(intermediate_directory.c_str());

    FileUtils::write_file_atomically(output_path, "data: 12345", 0600,
                                     intermediate_directory);
    ASSERT_TRUE(FileUtils::is_regular_file(output_path.c_str()));

    // The intermediate file was deleted:
    ASSERT_TRUE(FileUtils::directory_is_empty(intermediate_directory.c_str()));
}
