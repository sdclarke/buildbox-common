// Copyright 2019 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_merklize.h>
#include <gtest/gtest.h>

using namespace buildboxcommon;

TEST(FileTest, ToFilenode)
{
    File file;
    file.d_digest.set_hash("HASH HERE");
    file.d_digest.set_size_bytes(123);
    file.d_executable = true;

    auto fileNode = file.to_filenode(std::string("file.name"));

    EXPECT_EQ(fileNode.name(), "file.name");
    EXPECT_EQ(fileNode.digest().hash(), "HASH HERE");
    EXPECT_EQ(fileNode.digest().size_bytes(), 123);
    EXPECT_TRUE(fileNode.is_executable());
    EXPECT_FALSE(fileNode.has_node_properties());
}

TEST(NestedDirectoryTest, EmptyNestedDirectory)
{
    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = NestedDirectory().to_digest(&digestMap);
    EXPECT_EQ(1, digestMap.size());
    ASSERT_EQ(1, digestMap.count(digest));

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);
    EXPECT_EQ(0, message.files_size());
    EXPECT_EQ(0, message.directories_size());
}

TEST(NestedDirectoryTest, TrivialNestedDirectory)
{
    File file;
    file.d_digest.set_hash("DIGESTHERE");

    NestedDirectory directory;
    directory.add(file, "sample");

    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = directory.to_digest(&digestMap);
    EXPECT_EQ(1, digestMap.size());
    ASSERT_EQ(1, digestMap.count(digest));

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);
    EXPECT_EQ(0, message.directories_size());
    ASSERT_EQ(1, message.files_size());
    EXPECT_EQ("sample", message.files(0).name());
    EXPECT_EQ("DIGESTHERE", message.files(0).digest().hash());
}

TEST(NestedDirectoryTest, Subdirectories)
{
    File file;
    file.d_digest.set_hash("HASH1");

    File file2;
    file2.d_digest.set_hash("HASH2");

    NestedDirectory directory;
    directory.add(file, "sample");
    directory.add(file2, "subdir/anothersubdir/sample2");

    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = directory.to_digest(&digestMap);
    EXPECT_EQ(3, digestMap.size());
    ASSERT_EQ(1, digestMap.count(digest));

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);

    EXPECT_EQ(1, message.files_size());
    EXPECT_EQ("sample", message.files(0).name());
    EXPECT_EQ("HASH1", message.files(0).digest().hash());
    ASSERT_EQ(1, message.directories_size());
    EXPECT_EQ("subdir", message.directories(0).name());

    ASSERT_EQ(1, digestMap.count(message.directories(0).digest()));
    buildboxcommon::Directory subdir1;
    subdir1.ParseFromString(digestMap[message.directories(0).digest()]);
    EXPECT_EQ(0, subdir1.files_size());
    ASSERT_EQ(1, subdir1.directories_size());
    EXPECT_EQ("anothersubdir", subdir1.directories(0).name());

    ASSERT_EQ(1, digestMap.count(subdir1.directories(0).digest()));
    buildboxcommon::Directory subdir2;
    subdir2.ParseFromString(digestMap[subdir1.directories(0).digest()]);
    EXPECT_EQ(0, subdir2.directories_size());
    ASSERT_EQ(1, subdir2.files_size());
    EXPECT_EQ("sample2", subdir2.files(0).name());
    EXPECT_EQ("HASH2", subdir2.files(0).digest().hash());
}

TEST(NestedDirectoryTest, AddSingleDirectory)
{
    NestedDirectory directory;
    directory.addDirectory("foo");

    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = directory.to_digest(&digestMap);

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);
    EXPECT_EQ(0, message.files_size());
    ASSERT_EQ(1, message.directories_size());
    EXPECT_EQ("foo", message.directories(0).name());
}

TEST(NestedDirectoryTest, AddSlashDirectory)
{
    NestedDirectory directory;
    directory.addDirectory("/");

    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = directory.to_digest(&digestMap);

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);
    EXPECT_EQ(0, message.files_size());
    ASSERT_EQ(0, message.directories_size());
}

TEST(NestedDirectoryTest, AddAbsoluteDirectory)
{
    NestedDirectory directory;
    directory.addDirectory("/root");

    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = directory.to_digest(&digestMap);

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);
    EXPECT_EQ(0, message.files_size());
    ASSERT_EQ(1, message.directories_size());
    EXPECT_EQ("root", message.directories(0).name());
}

TEST(NestedDirectoryTest, EmptySubdirectories)
{

    NestedDirectory directory;
    directory.addDirectory("foo/bar/baz");

    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = directory.to_digest(&digestMap);

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);
    EXPECT_EQ(0, message.files_size());
    ASSERT_EQ(1, message.directories_size());
    EXPECT_EQ("foo", message.directories(0).name());

    buildboxcommon::Directory subdir;
    subdir.ParseFromString(digestMap[message.directories(0).digest()]);
    EXPECT_EQ(0, message.files_size());
    EXPECT_EQ(1, subdir.directories_size());
    EXPECT_EQ("bar", subdir.directories(0).name());

    buildboxcommon::Directory subdir2;
    subdir.ParseFromString(digestMap[subdir.directories(0).digest()]);
    EXPECT_EQ(0, message.files_size());
    EXPECT_EQ(1, subdir.directories_size());
    EXPECT_EQ("baz", subdir.directories(0).name());
}

TEST(NestedDirectoryTest, AddDirsToExistingNestedDirectory)
{
    File file;
    file.d_digest.set_hash("DIGESTHERE");

    NestedDirectory directory;
    directory.add(file, "directory/file");
    directory.addDirectory("directory/foo");
    directory.addDirectory("otherdir");

    std::unordered_map<buildboxcommon::Digest, std::string> digestMap;
    auto digest = directory.to_digest(&digestMap);

    buildboxcommon::Directory message;
    message.ParseFromString(digestMap[digest]);
    EXPECT_EQ(0, message.files_size());
    ASSERT_EQ(2, message.directories_size());
    EXPECT_EQ("directory", message.directories(0).name());
    EXPECT_EQ("otherdir", message.directories(1).name());

    buildboxcommon::Directory subdir;
    subdir.ParseFromString(digestMap[message.directories(0).digest()]);
    EXPECT_EQ(1, subdir.files_size());
    EXPECT_EQ(1, subdir.directories_size());
    EXPECT_EQ("file", subdir.files(0).name());
    EXPECT_EQ("foo", subdir.directories(0).name());
}

TEST(NestedDirectoryTest, SubdirectoriesToTree)
{
    File file;
    file.d_digest.set_hash("HASH1");

    File file2;
    file2.d_digest.set_hash("HASH2");

    NestedDirectory directory;
    directory.add(file, "sample");
    directory.add(file2, "subdir/anothersubdir/sample2");

    auto tree = directory.to_tree();
    EXPECT_EQ(2, tree.children_size());

    std::unordered_map<buildboxcommon::Digest, buildboxcommon::Directory>
        digestMap;
    for (auto &child : tree.children()) {
        digestMap[make_digest(child)] = child;
    }

    auto root = tree.root();

    EXPECT_EQ(1, root.files_size());
    EXPECT_EQ("sample", root.files(0).name());
    EXPECT_EQ("HASH1", root.files(0).digest().hash());
    ASSERT_EQ(1, root.directories_size());
    EXPECT_EQ("subdir", root.directories(0).name());

    ASSERT_EQ(1, digestMap.count(root.directories(0).digest()));
    buildboxcommon::Directory subdir1 =
        digestMap[root.directories(0).digest()];
    EXPECT_EQ(0, subdir1.files_size());
    ASSERT_EQ(1, subdir1.directories_size());
    EXPECT_EQ("anothersubdir", subdir1.directories(0).name());

    ASSERT_EQ(1, digestMap.count(subdir1.directories(0).digest()));
    buildboxcommon::Directory subdir2 =
        digestMap[subdir1.directories(0).digest()];
    EXPECT_EQ(0, subdir2.directories_size());
    ASSERT_EQ(1, subdir2.files_size());
    EXPECT_EQ("sample2", subdir2.files(0).name());
    EXPECT_EQ("HASH2", subdir2.files(0).digest().hash());
}

TEST(NestedDirectoryTest, MakeNestedDirectory)
{
    std::unordered_map<buildboxcommon::Digest, std::string> fileMap;
    auto nestedDirectory = make_nesteddirectory(".", &fileMap);

    EXPECT_EQ(1, nestedDirectory.d_subdirs->size());
    EXPECT_EQ(2, nestedDirectory.d_files.size());
    EXPECT_EQ(1, nestedDirectory.d_symlinks.size());

    EXPECT_EQ(
        "abc",
        FileUtils::getFileContents(
            fileMap[nestedDirectory.d_files["abc.txt"].d_digest].c_str()));

    EXPECT_EQ("abc.txt", nestedDirectory.d_symlinks["symlink"]);

    auto subdirectory = &(*nestedDirectory.d_subdirs)["subdir"];
    EXPECT_EQ(0, subdirectory->d_subdirs->size());
    EXPECT_EQ(1, subdirectory->d_files.size());
    EXPECT_EQ(0, subdirectory->d_symlinks.size());
    EXPECT_EQ("abc",
              FileUtils::getFileContents(
                  fileMap[subdirectory->d_files["abc.txt"].d_digest].c_str()));
}

TEST(NestedDirectoryTest, MakeNestedDirectoryFollowingSymlinks)
{
    std::unordered_map<buildboxcommon::Digest, std::string> fileMap;

    const bool followSymlinks = true;
    auto nestedDirectory = make_nesteddirectory(".", &fileMap, followSymlinks);

    EXPECT_EQ(1, nestedDirectory.d_subdirs->size());
    EXPECT_EQ(2 + 1, nestedDirectory.d_files.size());
    // The symlink was resolved.
    EXPECT_TRUE(nestedDirectory.d_symlinks.empty());

    EXPECT_EQ(
        "abc",
        FileUtils::getFileContents(
            fileMap[nestedDirectory.d_files["abc.txt"].d_digest].c_str()));

    EXPECT_EQ(
        "abc",
        FileUtils::getFileContents(
            fileMap[nestedDirectory.d_files["symlink"].d_digest].c_str()));

    auto subdirectory = &(*nestedDirectory.d_subdirs)["subdir"];
    EXPECT_EQ(0, subdirectory->d_subdirs->size());
    EXPECT_EQ(1, subdirectory->d_files.size());
    EXPECT_EQ(0, subdirectory->d_symlinks.size());

    EXPECT_EQ("abc",
              FileUtils::getFileContents(
                  fileMap[subdirectory->d_files["abc.txt"].d_digest].c_str()));
}

// Make sure the digest is calculated correctly regardless of the order in
// which the files are added. Important for caching.
TEST(NestedDirectoryTest, ConsistentDigestRegardlessOfFileOrder)
{
    int N = 5;
    // Get us some mock files
    File files[N];
    for (int i = 0; i < N; i++) {
        files[i].d_digest.set_hash("HASH_" + std::to_string(i));
    }

    // Create Nested Directory and add everything in-order
    NestedDirectory directory1;
    for (int i = 0; i < N; i++) {
        std::string fn =
            "subdir_" + std::to_string(i) + "/file_" + std::to_string(i);
        directory1.add(files[i], fn.c_str());
    }

    // Create another Nested Directory and add everything in reverse order
    NestedDirectory directory2;
    for (int i = N - 1; i >= 0; i--) {
        std::string fn =
            "subdir_" + std::to_string(i) + "/file_" + std::to_string(i);
        directory2.add(files[i], fn.c_str());
    }

    // Make sure the actual digests of those two directories are identical
    EXPECT_EQ(directory1.to_digest().hash(), directory2.to_digest().hash());
}

// Make sure digests of directories containing different files are different
TEST(NestedDirectoryTest, NestedDirectoryDigestsReallyBasedOnFiles)
{
    int N = 5;
    // Get us some mock files
    File files_dir1[N]; // Files to add in the first directory
    File files_dir2[N]; // Files to add in the second directory
    for (int i = 0; i < N; i++) {
        files_dir1[i].d_digest.set_hash("HASH_DIR1_" + std::to_string(i));
        files_dir2[i].d_digest.set_hash("HASH_DIR2_" + std::to_string(i));
    }

    // Create Nested Directories and add everything in-order
    NestedDirectory directory1, directory2;
    for (int i = 0; i < N; i++) {
        std::string fn =
            "subdir_" + std::to_string(i) + "/file_" + std::to_string(i);
        directory1.add(files_dir1[i], fn.c_str());
        directory2.add(files_dir2[i], fn.c_str());
    }

    // Make sure the digests are different
    EXPECT_NE(directory1.to_digest().hash(), directory2.to_digest().hash());
}
