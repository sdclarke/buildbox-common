/*
 * Copyright 2019 Bloomberg Finance LP
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

#include <buildboxcommon_mergeutil.h>

#include <buildboxcommon_cashash.h>
#include <buildboxcommon_protos.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;
using namespace testing;

class MergeFixture : public ::testing::Test {
  protected:
    MergeUtil::DirectoryTree d_emptyInputTree;
    MergeUtil::DirectoryTree d_inputTreeWithExecutableTrue;
    MergeUtil::DirectoryTree d_inputTreeWithExecutableFalse;
    MergeUtil::DirectoryTree d_inputTreeWithOverlapWithoutConflict;
    MergeUtil::DirectoryTree d_inputTreeWithOverlapWithConflict;
    MergeUtil::DirectoryTree d_chrootTemplateTree;

  public:
    typedef std::unordered_map<std::string, std::vector<std::string>>
        BasicTree;
    typedef std::vector<BasicTree> MerkleTree;
    typedef MerkleTree::iterator MerkleTreeItr;

    MergeFixture()
    {
        prepareEmptyInputTree(&d_emptyInputTree);
        prepareInputTree(&d_inputTreeWithExecutableTrue);
        prepareInputTree(&d_inputTreeWithExecutableFalse, false);
        prepareInputTreeWithOverlap(&d_inputTreeWithOverlapWithoutConflict);
        prepareInputTreeWithOverlap(&d_inputTreeWithOverlapWithConflict,
                                    "lib_so_contents_but_with_different_data");
        prepareTemplateTree(&d_chrootTemplateTree);
    }

    void prepareEmptyInputTree(MergeUtil::DirectoryTree *tree)
    {
        Directory empty_directory;
        const auto empty_directory_digest = make_digest(empty_directory);
        tree->emplace_back(empty_directory);
    }

    void prepareInputTree(MergeUtil::DirectoryTree *tree,
                          const bool isExecutable = true)
    {
        /* Creates the following directory structure:
         *
         * ./
         *   src/
         *       build.sh*
         *       headers/
         *               file1.h
         *               file2.h
         *               file3.h
         *       cpp/
         *           file1.cpp
         *           file2.cpp
         *           file3.cpp
         *           symlink: file4.cpp --> file3.cpp
         */

        // ./src/headers
        Directory headers_directory;
        std::vector<std::string> headerFiles = {"file1.h", "file2.h",
                                                "file3.h"};
        for (const auto &file : headerFiles) {
            FileNode *fileNode = headers_directory.add_files();
            fileNode->set_name(file);
            fileNode->set_is_executable(false);
            fileNode->mutable_digest()->CopyFrom(
                make_digest(file + "_contents"));
        }
        const auto headers_directory_digest = make_digest(headers_directory);

        // ./src/cpp
        Directory cpp_directory;
        std::vector<std::string> cppFiles = {"file1.cpp", "file2.cpp",
                                             "file3.cpp"};
        for (const auto &file : cppFiles) {
            FileNode *fileNode = cpp_directory.add_files();
            fileNode->set_name(file);
            fileNode->set_is_executable(false);
            fileNode->mutable_digest()->CopyFrom(
                make_digest(file + "_contents"));
        }
        SymlinkNode *symNode = cpp_directory.add_symlinks();
        symNode->set_name("file4.cpp");
        symNode->set_target("file3.cpp");
        const auto cpp_directory_digest = make_digest(cpp_directory);

        // ./src
        Directory src_directory;
        DirectoryNode *headersNode = src_directory.add_directories();
        headersNode->set_name("headers");
        headersNode->mutable_digest()->CopyFrom(headers_directory_digest);
        DirectoryNode *cppNode = src_directory.add_directories();
        cppNode->set_name("cpp");
        cppNode->mutable_digest()->CopyFrom(cpp_directory_digest);
        FileNode *fileNode = src_directory.add_files();
        fileNode->set_name("build.sh");
        fileNode->set_is_executable(isExecutable);
        fileNode->mutable_digest()->CopyFrom(make_digest("build.sh_contents"));
        const auto src_directory_digest = make_digest(src_directory);

        // .
        Directory root_directory;
        DirectoryNode *srcNode = root_directory.add_directories();
        srcNode->set_name("src");
        srcNode->mutable_digest()->CopyFrom(src_directory_digest);

        // create the tree
        tree->emplace_back(root_directory);
        tree->emplace_back(src_directory);
        tree->emplace_back(headers_directory);
        tree->emplace_back(cpp_directory);
    }

    void prepareInputTreeWithOverlap(
        MergeUtil::DirectoryTree *tree,
        const std::string &forcedCollisionData = "libc_so_contents")
    {
        /* Creates the following directory structure:
         *
         * ./
         *   src/
         *       headers/
         *               foo.h
         *       cpp/
         *           foo.cpp
         *   local/
         *         lib/
         *             libc.so
         */

        // ./src/headers
        Directory headers_directory;
        FileNode *headersFileNodes = headers_directory.add_files();
        headersFileNodes->set_name("foo.h");
        headersFileNodes->set_is_executable(false);
        headersFileNodes->mutable_digest()->CopyFrom(
            make_digest("foo_h_contents"));
        const auto headers_directory_digest = make_digest(headers_directory);

        // ./src/cpp
        Directory cpp_directory;
        FileNode *cppFileNodes = cpp_directory.add_files();
        cppFileNodes->set_name("foo.cpp");
        cppFileNodes->set_is_executable(false);
        cppFileNodes->mutable_digest()->CopyFrom(
            make_digest("foo_cpp_contents"));
        const auto cpp_directory_digest = make_digest(cpp_directory);

        // ./src
        Directory src_directory;
        DirectoryNode *headersNode = src_directory.add_directories();
        headersNode->set_name("headers");
        headersNode->mutable_digest()->CopyFrom(headers_directory_digest);
        DirectoryNode *cppNode = src_directory.add_directories();
        cppNode->set_name("cpp");
        cppNode->mutable_digest()->CopyFrom(cpp_directory_digest);
        const auto src_directory_digest = make_digest(src_directory);

        // ./lib/libc.so
        Directory lib_directory;
        FileNode *libFileNode = lib_directory.add_files();
        libFileNode->set_name("libc.so");
        libFileNode->set_is_executable(false);
        libFileNode->mutable_digest()->CopyFrom(
            make_digest(forcedCollisionData));
        const auto lib_directory_digest = make_digest(lib_directory);

        // ./local/lib
        Directory local_directory;
        DirectoryNode *libNode = local_directory.add_directories();
        libNode->set_name("lib");
        libNode->mutable_digest()->CopyFrom(lib_directory_digest);
        const auto local_directory_digest = make_digest(local_directory);

        // .
        Directory root_directory;
        DirectoryNode *srcNode = root_directory.add_directories();
        srcNode->set_name("src");
        srcNode->mutable_digest()->CopyFrom(src_directory_digest);
        DirectoryNode *localNode = root_directory.add_directories();
        localNode->set_name("local");
        localNode->mutable_digest()->CopyFrom(local_directory_digest);

        // create the tree
        tree->emplace_back(root_directory);
        tree->emplace_back(src_directory);
        tree->emplace_back(headers_directory);
        tree->emplace_back(cpp_directory);
        tree->emplace_back(local_directory);
        tree->emplace_back(lib_directory);
    }

    void prepareTemplateTree(MergeUtil::DirectoryTree *tree)
    {
        /* Creates the following directory structure:
         *
         * ./
         *   include/
         *           time.h
         *           sys/
         *               stat.h
         *   local/
         *         lib/
         *             libc.so
         *   var/
         */

        // ./include/sys
        Directory sys_directory;
        FileNode *sysFileNodes = sys_directory.add_files();
        sysFileNodes->set_name("stat.h");
        sysFileNodes->set_is_executable(false);
        sysFileNodes->mutable_digest()->CopyFrom(
            make_digest("stat_h_contents"));
        const auto sys_directory_digest = make_digest(sys_directory);

        // ./include
        Directory include_directory;
        FileNode *includeFileNodes = include_directory.add_files();
        includeFileNodes->set_name("time.h");
        includeFileNodes->set_is_executable(false);
        includeFileNodes->mutable_digest()->CopyFrom(
            make_digest("time_h_contents"));
        DirectoryNode *sysNode = include_directory.add_directories();
        sysNode->set_name("sys");
        sysNode->mutable_digest()->CopyFrom(sys_directory_digest);
        const auto include_directory_digest = make_digest(include_directory);

        // ./local/lib
        Directory lib_directory;
        FileNode *libFileNode = lib_directory.add_files();
        libFileNode->set_name("libc.so");
        libFileNode->set_is_executable(false);
        libFileNode->mutable_digest()->CopyFrom(
            make_digest("libc_so_contents"));
        const auto lib_directory_digest = make_digest(lib_directory);

        // ./local
        Directory local_directory;
        DirectoryNode *libNode = local_directory.add_directories();
        libNode->set_name("lib");
        libNode->mutable_digest()->CopyFrom(lib_directory_digest);
        const auto local_directory_digest = make_digest(local_directory);

        // ./var
        Directory var_directory;
        const auto var_directory_digest = make_digest(var_directory);

        // .
        Directory root_directory;
        // add include to root
        DirectoryNode *includeNode = root_directory.add_directories();
        includeNode->set_name("include");
        includeNode->mutable_digest()->CopyFrom(include_directory_digest);

        // add local to root
        DirectoryNode *localNode = root_directory.add_directories();
        localNode->set_name("local");
        localNode->mutable_digest()->CopyFrom(local_directory_digest);

        // add var to root
        DirectoryNode *varNode = root_directory.add_directories();
        varNode->set_name("var");
        varNode->mutable_digest()->CopyFrom(var_directory_digest);

        // create the tree
        tree->emplace_back(root_directory);
        tree->emplace_back(include_directory);
        tree->emplace_back(sys_directory);
        tree->emplace_back(local_directory);
        tree->emplace_back(lib_directory);
        tree->emplace_back(var_directory);
    }

    void print(const Digest &d, const Directory &directory)
    {
        const auto digest = CASHash::hash(directory.SerializeAsString());

        if (directory.files().empty() && directory.symlinks().empty() &&
            directory.directories().empty()) {
            std::cout << "Directory[" << 0 << "](" << digest
                      << ") contains zero files, zero symlinks and zero "
                         "subDirectories"
                      << std::endl;
            return;
        }

        // files
        const auto fileNodes = directory.files();
        for (int j = 0; j < fileNodes.size(); ++j) {
            std::cout << "Directory[" << 0 << "](" << digest
                      << ") --> FileNode[" << j << "]: name = \""
                      << fileNodes[j].name() << "\", digest = \""
                      << fileNodes[j].digest()
                      << "\", executable = " << std::boolalpha
                      << fileNodes[j].is_executable() << "\n";
        }

        // symlinks
        const auto symNodes = directory.symlinks();
        for (int j = 0; j < symNodes.size(); ++j) {
            std::cout << "Directory[" << 0 << "](" << digest
                      << ") --> SymlinkNode[" << j << "]: name = \""
                      << symNodes[j].name() << "\", target = \""
                      << symNodes[j].target() << "\""
                      << "\n";
        }

        // sub-directories
        const auto dirNodes = directory.directories();
        for (int j = 0; j < dirNodes.size(); ++j) {
            std::cout << "Directory[" << 0 << "](" << digest
                      << ") --> DirectoryNode[" << j << "]: name = \""
                      << dirNodes[j].name() << "\", digest = \""
                      << dirNodes[j].digest() << "\""
                      << "\n";
        }
    }

    void printMerkleTree(const MerkleTree &tree)
    {
        for (size_t i = 0; i < tree.size(); ++i) {
            for (const auto &it : tree.at(i)) {
                std::cout << it.first << " --> ";
                for (const auto &entry : it.second) {
                    std::cout << entry << ", ";
                }
                std::cout << std::endl;
            }
        }
    }

    // Recursively verify that a merkle tree matches an expected input layout.
    // This doesn't look at the hashes, just that the declared layout matches
    // (copied from RECC)
    void verify_merkle_tree(Digest digest, MerkleTreeItr expected,
                            MerkleTreeItr end, digest_string_map blobs)
    {
        ASSERT_NE(expected, end) << "Reached end of expected output early";
        auto current_blob = blobs.find(digest);
        ASSERT_NE(current_blob, blobs.end())
            << "No blob found for digest " << digest.hash();

        Directory directory;
        directory.ParseFromString(current_blob->second);

        /*
        print(digest, directory);
        for (const auto &vec : (*expected)) {
            for (const auto &file : vec.second) {
                std::cout << vec.first << ": " << file << std::endl;
            }
        }
        std::cout << "num_files = " << (*expected)["files"].size() <<
        std::endl; std::cout << "num_subdirs = " <<
        (*expected)["directories"].size() << std::endl;
        */

        // Exit early if there are more/less files or dirs in the given tree
        // than expected
        ASSERT_EQ(directory.files().size(), (*expected)["files"].size())
            << "Wrong number of files at current level";
        ASSERT_EQ(directory.directories().size(),
                  (*expected)["directories"].size())
            << "Wrong number of directories at current level";

        int f_index = 0;
        for (auto &file : directory.files()) {
            ASSERT_EQ(file.name(), (*expected)["files"][f_index])
                << "Wrong file found";
            f_index++;
        }
        int d_index = 0;
        for (auto &subdirectory : directory.directories()) {
            ASSERT_EQ(subdirectory.name(), (*expected)["directories"][d_index])
                << "Wrong directory found";
            d_index++;
        }
        // All the files/directories at this level are correct, now check all
        // the subdirectories
        for (auto &subdirectory : directory.directories()) {
            verify_merkle_tree(subdirectory.digest(), ++expected, end, blobs);
            ++expected;
        }
    }
};

// TEST CASES
TEST_F(MergeFixture, MergeSuccessEmptyInputTree)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    const bool result = MergeUtil::createMergedDigest(
        d_emptyInputTree, d_chrootTemplateTree, &mergedRootDigest, &dsMap);
    ASSERT_TRUE(result);

    MerkleTree expected_tree = {
        // top level, aka 'root'
        {{"directories", {"include", "local", "var"}}},
        // contents of 'include'
        {{"files", {"time.h"}}, {"directories", {"sys"}}},
        // contents of 'include/sys'
        {{"files", {"stat.h"}}},
        // contents of 'local'
        {{"directories", {"lib"}}},
        // contents of 'lib'
        {{"files", {"libc.so"}}},
        // contents of 'var'
        {{"directories", {}}}};
    verify_merkle_tree(mergedRootDigest, expected_tree.begin(),
                       expected_tree.end(), dsMap);
}

TEST_F(MergeFixture, MergeSuccessNoOverlap)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithExecutableTrue, d_chrootTemplateTree, &mergedRootDigest,
        &dsMap);
    ASSERT_TRUE(result);

    MerkleTree expected_tree = {
        // top level, aka 'root'
        {{"directories", {"include", "local", "src", "var"}}},
        // contents of 'include'
        {{"files", {"time.h"}}, {"directories", {"sys"}}},
        // contents of 'include/sys'
        {{"files", {"stat.h"}}},
        // contents of 'local'
        {{"directories", {"lib"}}},
        // contents of 'lib'
        {{"files", {"libc.so"}}},
        // contents of 'src'
        {{"files", {"build.sh"}}, {"directories", {"cpp", "headers"}}},
        // contents of 'cpp'
        {{"files", {"file1.cpp", "file2.cpp", "file3.cpp"}}},
        // contents of 'var'
        {{"directories", {}}},
        // contents of 'headers'
        {{"files", {"file1.h", "file2.h", "file3.h"}}},
    };
    verify_merkle_tree(mergedRootDigest, expected_tree.begin(),
                       expected_tree.end(), dsMap);
}

TEST_F(MergeFixture, MergeSuccessOverlapWithoutConflict)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithOverlapWithoutConflict, d_chrootTemplateTree,
        &mergedRootDigest, &dsMap);
    ASSERT_TRUE(result);

    MerkleTree expected_tree = {
        // top level, aka 'root'
        {{"directories", {"include", "local", "src", "var"}}},
        // contents of 'include'
        {{"files", {"time.h"}}, {"directories", {"sys"}}},
        // contents of 'include/sys'
        {{"files", {"stat.h"}}},
        // contents of 'local'
        {{"directories", {"lib"}}},
        // contents of 'lib'
        {{"files", {"libc.so"}}},
        // contents of 'src'
        {{"directories", {"cpp", "headers"}}},
        // contents of 'cpp'
        {{"files", {"foo.cpp"}}},
        // contents of 'var'
        {{"directories", {}}},
        // contents of 'headers'
        {{"files", {"foo.h"}}}};
    verify_merkle_tree(mergedRootDigest, expected_tree.begin(),
                       expected_tree.end(), dsMap);
}

TEST_F(MergeFixture, MergeFailOverlapWithConflict)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithOverlapWithConflict, d_chrootTemplateTree,
        &mergedRootDigest, &dsMap);
    ASSERT_FALSE(result);
}

TEST_F(MergeFixture, MergeMismatchIsExecutable)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithExecutableTrue, d_inputTreeWithExecutableFalse,
        &mergedRootDigest, &dsMap);
    ASSERT_FALSE(result);
}
