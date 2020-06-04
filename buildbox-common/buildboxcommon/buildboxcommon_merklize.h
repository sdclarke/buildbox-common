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

#ifndef INCLUDED_BUILDBOXCOMMON_MERKLIZE
#define INCLUDED_BUILDBOXCOMMON_MERKLIZE

#include <buildboxcommon_protos.h>

#include <google/protobuf/message.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace buildboxcommon {

typedef std::unordered_map<buildboxcommon::Digest, std::string>
    digest_string_map;

typedef std::unordered_map<buildboxcommon::Digest, std::string>::iterator
    digest_string_map_it;

typedef std::function<Digest(int fd)> FileDigestFunction;

/**
 * Represents a single file.
 */
struct File {
    Digest d_digest;
    bool d_executable;
    bool d_mtime_set = false;
    std::chrono::system_clock::time_point d_mtime;

    File(){};

    File(Digest digest, bool executable)
        : d_digest(digest), d_executable(executable){};

    /**
     * Constructs a File given the path to a file on disk.
     */
    File(const char *path,
         const std::vector<std::string> &capture_properties = {});
    File(const char *path, const FileDigestFunction &fileDigestFunc,
         const std::vector<std::string> &capture_properties = {});
    File(int dirfd, const char *path, const FileDigestFunction &fileDigestFunc,
         const std::vector<std::string> &capture_properties = {});

    /**
     * Converts a File to a FileNode with the given name.
     */
    FileNode to_filenode(const std::string &name) const;
};

/**
 * Represents a directory that, optionally, has other directories inside.
 */
struct NestedDirectory {
  public:
    // Important to use a sorted map to keep subdirectories ordered by name
    typedef std::map<std::string, NestedDirectory> subdir_map;
    typedef std::map<std::string, NestedDirectory>::iterator subdir_map_it;
    std::unique_ptr<subdir_map> d_subdirs;
    // Important to use a sorted map to keep files ordered by name
    std::map<std::string, File> d_files;
    std::map<std::string, std::string> d_symlinks;

    NestedDirectory() : d_subdirs(new subdir_map){};

    /**
     * Add the given File to this NestedDirectory at the given relative path,
     * which may include subdirectories.
     */
    void add(const File &file, const char *relativePath);

    /**
     * Add the given symlink to this NestedDirectory at the given relative
     * path, which may include subdirectories
     */
    void addSymlink(const std::string &target, const char *relativePath);

    /**
     * Add the given Directory to this NestedDirectory at a given relative
     * path. If the directory has contents, the add method should be used
     * instead
     */
    void addDirectory(const char *directory);

    /**
     * Convert this NestedDirectory to a Directory message and return its
     * Digest.
     *
     * If a digestMap is passed, serialized Directory messages corresponding to
     * this directory and its subdirectories will be stored in it using their
     * Digest messages as the keys. (This is recursive -- nested subdirectories
     * will also be stored.
     */
    Digest to_digest(digest_string_map *digestMap = nullptr) const;

    /**
     * Convert this NestedDirectory to a Tree message.
     */
    Tree to_tree() const;
};

/**
 * Create a Digest message from the given blob.
 */
Digest make_digest(const std::string &blob);

/**
 * Create a Digest message from the given proto message.
 */
inline Digest make_digest(const google::protobuf::MessageLite &message)
{
    return make_digest(message.SerializeAsString());
}

/**
 * Create a NestedDirectory containing the contents of the given path and its
 * subdirectories.
 *
 * If a fileMap is passed, paths to all files referenced by the NestedDirectory
 * will be stored in it using their Digest messages as the keys.
 */
NestedDirectory
make_nesteddirectory(const char *path, digest_string_map *fileMap = nullptr,
                     const std::vector<std::string> &capture_properties =
                         std::vector<std::string>());
NestedDirectory
make_nesteddirectory(const char *path,
                     const FileDigestFunction &fileDigestFunc,
                     digest_string_map *fileMap = nullptr,
                     const std::vector<std::string> &capture_properties =
                         std::vector<std::string>());

} // namespace buildboxcommon

#endif
