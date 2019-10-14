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

#include <buildboxcommon_mergeutil.h>

#include <buildboxcommon_cashash.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_merklize.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace buildboxcommon {

namespace {

class NodeMetaData {
  protected:
    const std::string d_path;
    explicit NodeMetaData(const std::string &path) : d_path(path) {}
    virtual ~NodeMetaData() {}

  public:
    const std::string &path() const { return d_path; }
    virtual const Digest &digest() const = 0;
    virtual void addToNestedDirectory(NestedDirectory *nd) const = 0;
    virtual bool isExecutable() const { return false; }
    virtual void print(std::ostream &out) const = 0;
};

bool operator==(const NodeMetaData &obj, const FileNode &fileNode)
{
    return (obj.digest() == fileNode.digest() &&
            obj.isExecutable() == fileNode.is_executable());
}

bool operator!=(const NodeMetaData &obj, const FileNode &fileNode)
{
    return !(obj == fileNode);
}

class FileNodeMetaData : public NodeMetaData {
  private:
    const File d_file;

  public:
    FileNodeMetaData(const std::string &path, const Digest &digest,
                     const bool is_executable)
        : NodeMetaData(path), d_file(digest, is_executable)
    {
    }

    const Digest &digest() const override { return d_file.d_digest; }

    void addToNestedDirectory(NestedDirectory *nd) const override
    {
        nd->add(d_file, d_path.c_str());
    }

    bool isExecutable() const override { return d_file.d_executable; }

    void print(std::ostream &out) const override
    {
        out << "file:    " << path() << " [" << digest()
            << ", executable = " << std::boolalpha << isExecutable() << "]\n";
    }
};

class SymlinkNodeMetaData : public NodeMetaData {
  private:
    const std::string d_symlinkTarget;

  public:
    SymlinkNodeMetaData(const std::string &symlinkName,
                        const std::string &symlinkTarget)
        : NodeMetaData(symlinkName), d_symlinkTarget(symlinkTarget)
    {
    }

    // symlinks have no digest, so return an empty one
    // see
    // https://gitlab.com/BuildGrid/buildbox/buildbox-common/blob/master/protos/build/bazel/remote/execution/v2/remote_execution.proto#L658
    const Digest &digest() const override
    {
        static Digest d;
        return d;
    }

    void addToNestedDirectory(NestedDirectory *nd) const override
    {
        nd->addSymlink(d_symlinkTarget, path().c_str());
    }

    void print(std::ostream &out) const override
    {
        out << "symlink: " << path() << ", " << d_symlinkTarget << "\n";
    }
};

class DirNodeMetaData : public NodeMetaData {
  private:
    const Digest d_digest;

  public:
    DirNodeMetaData(const std::string &path, const Digest &digest)
        : NodeMetaData(path), d_digest(digest)
    {
    }

    const Digest &digest() const override { return d_digest; }

    void addToNestedDirectory(NestedDirectory *nd) const override
    {
        nd->addDirectory(d_path.c_str());
    }

    void print(std::ostream &out) const override
    {
        out << "dir:     " << path() << " [" << digest() << "]\n";
    }
};

typedef std::unordered_map<std::string, std::shared_ptr<NodeMetaData>>
    PathNodeMetaDataMap;

std::ostream &operator<<(std::ostream &out, const NodeMetaData &obj)
{
    obj.print(out);
    return out;
}

inline std::string genNewPath(const std::string &dirName,
                              const std::string &nodeName)
{
    const std::string result =
        dirName.empty() ? nodeName : dirName + "/" + nodeName;
    return result;
}

/**
 * Create a string, one per file or directory by recursively iterating
 * over a chain of directories and subdirectores until arriving at the leaf
 * node. For example the following directory tree:
 *   src/
 *       headers/
 *               foo.h
 *       cpp/
 *           foo.cpp
 *   local/
 *         lib/
 *             libc.so
 *   var/
 *
 *  can be expressed as a series of path names which
 *  we can pass to the NestedDirectory::add() and
 *  NestedDirectory::addDirectory() methods:
 *
 *  NestedDirectory result;
 *  result.add("src/headers/foo.h");
 *  result.add("src/cpp/foo.cpp");
 *  result.add("local/lib/libc.so");
 *  result.addDirectory("var");
 */
void buildFlattenedPath(PathNodeMetaDataMap *map,
                        const buildboxcommon::Directory &directory,
                        const digest_string_map &dsMap,
                        const std::string &dirName = "")
{
    // files
    for (const auto &node : directory.files()) {
        const std::string newFile = genNewPath(dirName, node.name());

        // collision detection for files is defined as
        // same file name but different digest or 'is_executable' flag
        const auto it = map->find(newFile);
        if (it != map->end() && (*it->second != node)) {
            std::ostringstream oss;
            oss << "file collision: existing file [" << it->second->path()
                << ":" << it->second->digest() << ":" << std::boolalpha
                << it->second->isExecutable() << "]"
                << " detected while attempting to add new file [" << newFile
                << ":" << node.digest() << ":" << std::boolalpha
                << node.is_executable();
            throw std::runtime_error(oss.str());
        }
        map->emplace(newFile,
                     std::make_shared<FileNodeMetaData>(newFile, node.digest(),
                                                        node.is_executable()));
    }

    // symlinks
    for (const auto &node : directory.symlinks()) {
        const std::string newName = genNewPath(dirName, node.name());
        const std::string key = newName + ":" + node.target();

        // collision detection for symlinks is defined as
        // same name and target
        const auto it = map->find(key);
        if (it != map->end()) {
            std::ostringstream oss;
            oss << "symlink collision: existing name/target \"" << it->first
                << "\" detected while attempting to add new name/target \""
                << key << "\"";
            throw std::runtime_error(oss.str());
        }
        map->emplace(key, std::make_shared<SymlinkNodeMetaData>(
                              newName, node.target()));
    }

    // subdirectories
    // no collision detection is needed at this level because we allow
    // directories with the same name in the merged output; if there are
    // collisions in the subdirectory data, it will be detected at the file and
    // symlink level
    for (const auto &node : directory.directories()) {
        const std::string newDirectoryPath = genNewPath(dirName, node.name());

        map->emplace(newDirectoryPath, std::make_shared<DirNodeMetaData>(
                                           newDirectoryPath, node.digest()));

        const auto subDirIt = dsMap.find(node.digest());
        if (subDirIt == dsMap.end()) {
            BUILDBOX_LOG_ERROR("error finding digest " << node.digest());
            continue;
        }
        Directory nextDir;
        nextDir.ParseFromString(subDirIt->second);
        buildFlattenedPath(map, nextDir, dsMap, newDirectoryPath);
    }
}

void buildDigestDirectoryMap(
    const buildboxcommon::MergeUtil::DirectoryTree &tree,
    digest_string_map *dsMap)
{
    for (const auto &directory : tree) {
        const auto serialized = directory.SerializeAsString();
        const auto digest = buildboxcommon::make_digest(serialized);
        if (!dsMap->emplace(digest, serialized).second) {
            BUILDBOX_LOG_DEBUG("digest ["
                               << digest
                               << "] already exists(which is "
                                  "allowable due to having the same digest)");
        }
    }
}

} // namespace

bool MergeUtil::createMergedDigest(const DirectoryTree &inputTree,
                                   const DirectoryTree &templateTree,
                                   Digest *rootDigest,
                                   digest_string_map *dsMap)
{
    if (inputTree.empty() && templateTree.empty()) {
        BUILDBOX_LOG_ERROR("invalid args: both input trees are empty");
        return false;
    }

    // build a mapping that maps all Directory entries by their digests
    buildDigestDirectoryMap(inputTree, dsMap);
    buildDigestDirectoryMap(templateTree, dsMap);

    // Create a map of full pathnames and while doing so, detect
    // collisions, which we define as files/directories with the
    // same name but with different digests
    // eg:
    // proj/src/file.cpp
    // proj/headers/file.h
    PathNodeMetaDataMap map;
    try {
        buildFlattenedPath(&map, inputTree.at(0), *dsMap);
        buildFlattenedPath(&map, templateTree.at(0), *dsMap);
    }
    catch (const std::runtime_error &e) {
        BUILDBOX_LOG_ERROR(e.what());
        return false;
    }

    // Iterate over the list of file/directory paths
    // and use the NestedDirectory component to
    // build a merged directory tree
    NestedDirectory result;
    for (const auto &it : map) {
        it.second->addToNestedDirectory(&result);
    }

    // Iterate over all the dirs/file and generate a new
    // merged root digest
    *rootDigest = result.to_digest(dsMap);

    return true;
}

std::ostream &operator<<(std::ostream &out,
                         const MergeUtil::DirectoryTree &tree)
{
    if (tree.empty()) {
        return out;
    }

    // build a mapping that maps all Directory entries by their digests
    digest_string_map dsMap;
    buildDigestDirectoryMap(tree, &dsMap);

    PathNodeMetaDataMap map;
    buildFlattenedPath(&map, tree.at(0), dsMap);
    for_each(map.begin(), map.end(),
             [&out](const PathNodeMetaDataMap::value_type &p) {
                 out << *p.second;
             });
    return out;
}

std::ostream &operator<<(
    std::ostream &out,
    const ::google::protobuf::RepeatedPtrField<buildboxcommon::Directory>
        &tree)
{
    for (int i = 0; i < tree.size(); ++i) {
        const auto &directory = tree.Get(i);
        const auto digest = CASHash::hash(directory.SerializeAsString());

        // files
        const auto fileNodes = directory.files();
        for (int j = 0; j < fileNodes.size(); ++j) {
            out << "Directory[" << i << "](" << digest << ") --> FileNode["
                << j << "]: name = \"" << fileNodes[j].name()
                << "\", digest = \"" << fileNodes[j].digest()
                << "\", executable = " << std::boolalpha
                << fileNodes[j].is_executable() << "\n";
        }

        // symlinks
        const auto symNodes = directory.symlinks();
        for (int j = 0; j < symNodes.size(); ++j) {
            out << "Directory[" << i << "](" << digest << ") --> SymlinkNode["
                << j << "]: name = \"" << symNodes[j].name()
                << "\", target = \"" << symNodes[j].target() << "\""
                << "\n";
        }

        // sub-directories
        const auto dirNodes = directory.directories();
        for (int j = 0; j < dirNodes.size(); ++j) {
            out << "Directory[" << i << "](" << digest
                << ") --> DirectoryNode[" << j << "]: name = \""
                << dirNodes[j].name() << "\", digest = \""
                << dirNodes[j].digest() << "\""
                << "\n";
        }
    }

    return out;
}

} // namespace buildboxcommon
