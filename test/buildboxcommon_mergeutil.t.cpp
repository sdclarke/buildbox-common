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
    MergeUtil::DirectoryTree d_inputTreeWithSymlinks;
    MergeUtil::DirectoryTree d_chrootTemplateTree;
    MergeUtil::DirectoryTree d_chrootTemplateTreeWithSymlinkCollision;
    MergeUtil::DirectoryTree d_chrootTemplateTreeWithoutSymlinkCollision;

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

        // symlink specific test cases
        prepareInputTreeWithSymlinks(&d_inputTreeWithSymlinks);
        prepareTemplateTreeWithSymlinkCollision(
            &d_chrootTemplateTreeWithSymlinkCollision);
        prepareTemplateTreeWithoutSymlinkCollision(
            &d_chrootTemplateTreeWithoutSymlinkCollision);
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

    void prepareInputTreeWithSymlinks(MergeUtil::DirectoryTree *tree)
    {
        /* Creates the following directory structure:
         *
         * ./
         *   include/
         *           headers1/
         *                   file1.h
         *           headers2/
         *                   file2.h --> ../headers1/file1.h
         */

        // ./include/headers1
        Directory headers1_directory;
        FileNode *fileNode = headers1_directory.add_files();
        fileNode->set_name("file1.h");
        fileNode->set_is_executable(false);
        fileNode->mutable_digest()->CopyFrom(make_digest("file1_h_contents"));
        const auto headers1_directory_digest = make_digest(headers1_directory);

        // ./include/headers2
        Directory headers2_directory;
        SymlinkNode *symNode = headers2_directory.add_symlinks();
        symNode->set_name("file2.h");
        symNode->set_target("../headers1/file1.cpp");
        const auto headers2_directory_digest = make_digest(headers2_directory);

        // ./include
        Directory include_directory;
        DirectoryNode *headers1Node = include_directory.add_directories();
        headers1Node->set_name("headers1");
        headers1Node->mutable_digest()->CopyFrom(headers1_directory_digest);
        DirectoryNode *headers2Node = include_directory.add_directories();
        headers2Node->set_name("headers2");
        headers2Node->mutable_digest()->CopyFrom(headers2_directory_digest);
        const auto include_directory_digest = make_digest(include_directory);

        // .
        Directory root_directory;
        DirectoryNode *includeNode = root_directory.add_directories();
        includeNode->set_name("include");
        includeNode->mutable_digest()->CopyFrom(include_directory_digest);

        // create the tree
        tree->emplace_back(root_directory);
        tree->emplace_back(include_directory);
        tree->emplace_back(headers1_directory);
        tree->emplace_back(headers2_directory);
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

    void
    prepareTemplateTreeWithSymlinkCollision(MergeUtil::DirectoryTree *tree)
    {
        /* Creates the following directory structure:
         *
         * ./
         *   include/
         *           time.h
         *           headers1/
         *                    file.h
         *           headers2/
         *                    file2.h --> ../headers1/file.h
         *   local/
         *         lib/
         *             libc.so
         *   var/
         */

        // ./include/headers1
        Directory headers1_directory;
        FileNode *fileNode = headers1_directory.add_files();
        fileNode->set_name("file.h");
        fileNode->set_is_executable(false);
        fileNode->mutable_digest()->CopyFrom(make_digest("file_h_contents"));
        const auto headers1_directory_digest = make_digest(headers1_directory);

        // ./include/headers2
        Directory headers2_directory;
        SymlinkNode *symNode = headers2_directory.add_symlinks();
        symNode->set_name("file2.h");
        symNode->set_target("../headers1/file.cpp");
        const auto headers2_directory_digest = make_digest(headers2_directory);

        // ./include
        Directory include_directory;
        FileNode *includeFileNodes = include_directory.add_files();
        includeFileNodes->set_name("time.h");
        includeFileNodes->set_is_executable(false);
        includeFileNodes->mutable_digest()->CopyFrom(
            make_digest("time_h_contents"));
        DirectoryNode *headers1Node = include_directory.add_directories();
        headers1Node->set_name("headers1");
        headers1Node->mutable_digest()->CopyFrom(headers1_directory_digest);
        DirectoryNode *headers2Node = include_directory.add_directories();
        headers2Node->set_name("headers2");
        headers2Node->mutable_digest()->CopyFrom(headers2_directory_digest);
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
        tree->emplace_back(headers1_directory);
        tree->emplace_back(headers2_directory);
        tree->emplace_back(local_directory);
        tree->emplace_back(lib_directory);
        tree->emplace_back(var_directory);
    }

    void
    prepareTemplateTreeWithoutSymlinkCollision(MergeUtil::DirectoryTree *tree)
    {
        /* Creates the following directory structure:
         *
         * ./
         *   include/
         *           time.h
         *           headers1/
         *                    file1.h
         *           headers2/
         *                    file2.h --> ../headers1/file1.h
         *   local/
         *         lib/
         *             libc.so
         *   var/
         */

        // ./include/headers1
        Directory headers1_directory;
        FileNode *fileNode = headers1_directory.add_files();
        fileNode->set_name("file1.h");
        fileNode->set_is_executable(false);
        fileNode->mutable_digest()->CopyFrom(make_digest("file1_h_contents"));
        const auto headers1_directory_digest = make_digest(headers1_directory);

        // ./include/headers2
        Directory headers2_directory;
        SymlinkNode *symNode = headers2_directory.add_symlinks();
        symNode->set_name("file2.h");
        symNode->set_target("../headers1/file1.cpp");
        const auto headers2_directory_digest = make_digest(headers2_directory);

        // ./include
        Directory include_directory;
        FileNode *includeFileNodes = include_directory.add_files();
        includeFileNodes->set_name("time.h");
        includeFileNodes->set_is_executable(false);
        includeFileNodes->mutable_digest()->CopyFrom(
            make_digest("time_h_contents"));
        DirectoryNode *headers1Node = include_directory.add_directories();
        headers1Node->set_name("headers1");
        headers1Node->mutable_digest()->CopyFrom(headers1_directory_digest);
        DirectoryNode *headers2Node = include_directory.add_directories();
        headers2Node->set_name("headers2");
        headers2Node->mutable_digest()->CopyFrom(headers2_directory_digest);
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
        tree->emplace_back(headers1_directory);
        tree->emplace_back(headers2_directory);
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
            const BasicTree &basicTree = tree.at(i);
            for (const auto &it : basicTree) {
                std::cout << "tree[" << i << "," << std::hex
                          << (void *)&basicTree << "]: " << it.first
                          << " --> ";
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
    void verify_merkle_tree(const Digest &digest, MerkleTree &tree, int &index,
                            int end, const digest_string_map &blobs,
                            const bool verbose = false)
    {
        ASSERT_NE(index, end) << "Reached end of expected output early";
        auto current_blob = blobs.find(digest);
        ASSERT_NE(current_blob, blobs.end())
            << "No blob found for digest " << digest.hash_other();

        Directory directory;
        directory.ParseFromString(current_blob->second);

        if (verbose) {
            if (index == 0) {
                printMerkleTree(tree);
            }

            std::cout << std::hex << (void *)&tree.at(index) << std::endl;
            print(digest, directory);
            for (const auto &it : tree.at(index)) {
                for (const auto &entry : it.second) {
                    std::cout << it.first << ": " << entry << std::endl;
                }
            }
            std::cout << "num_files = " << tree[index]["files"].size()
                      << std::endl;
            std::cout << "num_symlinks = " << tree[index]["symlinks"].size()
                      << std::endl;
            std::cout << "num_subdirs = " << tree[index]["directories"].size()
                      << std::endl;
        }

        // Exit early if there are more/less files or dirs in the given tree
        // than expected
        ASSERT_EQ(directory.files().size(), tree[index]["files"].size())
            << "Wrong number of files at current level";
        ASSERT_EQ(directory.symlinks().size(), tree[index]["symlinks"].size())
            << "Wrong number of symlinks at current level";
        ASSERT_EQ(directory.directories().size(),
                  tree[index]["directories"].size())
            << "Wrong number of directories at current level";

        int f_index = 0;
        for (auto &file : directory.files()) {
            ASSERT_EQ(file.name(), tree[index]["files"][f_index])
                << "Wrong file found";
            f_index++;
        }
        int d_index = 0;
        for (auto &subdirectory : directory.directories()) {
            ASSERT_EQ(subdirectory.name(), tree[index]["directories"][d_index])
                << "Wrong directory found";
            d_index++;
        }
        // All the files/directories at this level are correct, now check all
        // the subdirectories
        for (auto &subdirectory : directory.directories()) {
            if (verbose) {
                std::cout << "checking subdirectories" << std::endl;
            }
            verify_merkle_tree(subdirectory.digest(), tree, ++index, end,
                               blobs, verbose);
        }
    }

    // Verify that the populated `mergedDirectoryDigests` only contains
    // digests from merged directories. This is done by checking it
    // against the diff of the merged tree, and the union of the
    // template & input trees.
    void verify_merged_directory_list(
        const MergeUtil::DirectoryTree &inputTree,
        const MergeUtil::DirectoryTree &templateTree,
        const digest_string_map &newDirectoryBlobs,
        const MergeUtil::DigestVector &mergedDirectoryDigests)
    {
        std::set<std::string> newDirSet;
        std::set<std::string> oldDirSet;
        std::set<std::string> mergeDirSet;

        for (const auto &directory : inputTree) {
            const auto serialized = directory.SerializeAsString();
            const auto digest = buildboxcommon::make_digest(serialized);
            oldDirSet.emplace(toString(digest));
        }
        for (const auto &directory : templateTree) {
            const auto serialized = directory.SerializeAsString();
            const auto digest = buildboxcommon::make_digest(serialized);
            oldDirSet.emplace(toString(digest));
        }
        for (const auto &it : newDirectoryBlobs) {
            newDirSet.emplace(toString(it.first));
        }
        for (const auto &it : mergedDirectoryDigests) {
            mergeDirSet.emplace(toString(it));
        }

        // Using set_difference to cross check logic in createMergedDigest
        // Which constructs the diff using comparisons between unordered maps
        std::set<std::string> diffSet;
        std::set_difference(newDirSet.cbegin(), newDirSet.cend(),
                            oldDirSet.cbegin(), oldDirSet.cend(),
                            std::inserter(diffSet, diffSet.end()));

        ASSERT_EQ(diffSet, mergeDirSet);
    }
};

// TEST CASES
TEST_F(MergeFixture, MergeSuccessEmptyInputTree)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    MergeUtil::DigestVector mergedDirectoryList;
    const bool result = MergeUtil::createMergedDigest(
        d_emptyInputTree, d_chrootTemplateTree, &mergedRootDigest, &dsMap,
        &mergedDirectoryList);
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

    int startIndex = 0;
    verify_merkle_tree(mergedRootDigest, expected_tree, startIndex,
                       expected_tree.size(), dsMap);
    verify_merged_directory_list(d_emptyInputTree, d_chrootTemplateTree, dsMap,
                                 mergedDirectoryList);
}

TEST_F(MergeFixture, MergeSuccessNoOverlap)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    MergeUtil::DigestVector mergedDirectoryList;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithExecutableTrue, d_chrootTemplateTree, &mergedRootDigest,
        &dsMap, &mergedDirectoryList);
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
        {{"files", {"file1.cpp", "file2.cpp", "file3.cpp"}},
         {"symlinks", {"file4.h"}}},

        // contents of 'headers'
        {{"files", {"file1.h", "file2.h", "file3.h"}}},

        // contents of 'var'
        {{"directories", {}}}};

    int startIndex = 0;

    verify_merkle_tree(mergedRootDigest, expected_tree, startIndex,
                       expected_tree.size(), dsMap);
    verify_merged_directory_list(d_inputTreeWithExecutableTrue,
                                 d_chrootTemplateTree, dsMap,
                                 mergedDirectoryList);
}

TEST_F(MergeFixture, MergeSuccessOverlapWithoutConflict)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    MergeUtil::DigestVector mergedDirectoryList;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithOverlapWithoutConflict, d_chrootTemplateTree,
        &mergedRootDigest, &dsMap, &mergedDirectoryList);
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

        // contents of 'headers'
        {{"files", {"foo.h"}}},

        // contents of 'var'
        {{"directories", {}}}};

    int startIndex = 0;
    verify_merkle_tree(mergedRootDigest, expected_tree, startIndex,
                       expected_tree.size(), dsMap);
    verify_merged_directory_list(d_inputTreeWithOverlapWithoutConflict,
                                 d_chrootTemplateTree, dsMap,
                                 mergedDirectoryList);
}

// To test backwards compatibility
// where mergedDirectoryList == nullptr by default.
TEST_F(MergeFixture, MergeSuccessOverlapWithoutConflictNullptr)
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

        // contents of 'headers'
        {{"files", {"foo.h"}}},

        // contents of 'var'
        {{"directories", {}}}};

    int startIndex = 0;
    verify_merkle_tree(mergedRootDigest, expected_tree, startIndex,
                       expected_tree.size(), dsMap);
}

TEST_F(MergeFixture, MergeFailOverlapWithConflict)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    MergeUtil::DigestVector mergedDirectoryList;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithOverlapWithConflict, d_chrootTemplateTree,
        &mergedRootDigest, &dsMap, &mergedDirectoryList);
    ASSERT_FALSE(result);
}

// To test backwards compatibility
// // where mergedDirectoryList == nullptr by default.
TEST_F(MergeFixture, MergeFailOverlapWithConflictNullptr)
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
    MergeUtil::DigestVector mergedDirectoryList;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithExecutableTrue, d_inputTreeWithExecutableFalse,
        &mergedRootDigest, &dsMap, &mergedDirectoryList);
    ASSERT_FALSE(result);
}

TEST_F(MergeFixture, MergeSuccessSymlinkCollision)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    MergeUtil::DigestVector mergedDirectoryList;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithSymlinks, d_chrootTemplateTreeWithoutSymlinkCollision,
        &mergedRootDigest, &dsMap, &mergedDirectoryList);
    ASSERT_TRUE(result);

    MerkleTree expected_tree = {
        // top level, aka 'root'
        {{"directories", {"include", "local", "var"}}},

        // contents of 'include'
        {{"files", {"time.h"}}, {"directories", {"headers1", "headers2"}}},

        // contents of 'include/headers1'
        {{"files", {"file1.h"}}},

        // contents of 'include/headers2'
        {{"symlinks", {"file2.h"}}},

        // contents of 'local'
        {{"directories", {"lib"}}},

        // contents of 'lib'
        {{"files", {"libc.so"}}},

        // contents of 'var'
        {{"directories", {}}},
    };

    int startingIndex = 0;
    verify_merkle_tree(mergedRootDigest, expected_tree, startingIndex,
                       expected_tree.size(), dsMap);
    verify_merged_directory_list(d_chrootTemplateTreeWithoutSymlinkCollision,
                                 d_chrootTemplateTree, dsMap,
                                 mergedDirectoryList);
}

TEST_F(MergeFixture, MergeFailureSymlinkCollision)
{
    // merge
    Digest mergedRootDigest;
    buildboxcommon::digest_string_map dsMap;
    MergeUtil::DigestVector mergedDirectoryList;
    const bool result = MergeUtil::createMergedDigest(
        d_inputTreeWithSymlinks, d_chrootTemplateTreeWithSymlinkCollision,
        &mergedRootDigest, &dsMap, &mergedDirectoryList);
    ASSERT_FALSE(result);
}
