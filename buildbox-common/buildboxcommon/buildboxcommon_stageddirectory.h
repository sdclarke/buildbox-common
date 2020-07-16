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

#ifndef INCLUDED_BUILDBOXCOMMON_STAGEDDIRECTORY
#define INCLUDED_BUILDBOXCOMMON_STAGEDDIRECTORY

#include <buildboxcommon_client.h>

#include <memory>
#include <string>

namespace buildboxcommon {

/**
 * Represents a directory that has been "staged", i.e. fetched from CAS and
 * made available in a local filesystem path.
 *
 * Currently there are two subclasses that employ different staging mechanisms:
 * `FallbackStagedDirectory` and `LocalCasStagedDirectory`. The latter relies
 * on the `StageTree()` call of the LocalCAS protocol.
 *
 */
class StagedDirectory {
  public:
    /**
     * Subclasses' constructors should download the staged directory to
     * an arbitrary location in the local filesystem, then store that location
     * in _path.
     */
    StagedDirectory() {}

    /**
     * Remove the staged directory from the filesystem.
     */
    inline virtual ~StagedDirectory(){};

    /**
     * Return the path (on the filesystem) where the downloaded files are
     * located.
     */
    inline const char *getPath() const { return d_path.c_str(); }

    // NOTE: The implementations of the capture methods below will assume that
    // changes will not take place in the input root while they are running.
    // This is generally true when they are called after a command finishes
    // executing, but, to avoid race conditions, other processes should be kept
    // from writing to those directories as well.

    // Capture a file inside the `Command`'s input root.
    virtual OutputFile captureFile(const char *relative_path,
                                   const Command &command) const = 0;

    // Capture a directory inside the `Command's` input root.
    virtual OutputDirectory captureDirectory(const char *relative_path,
                                             const Command &command) const = 0;

    /**
     * Capture all the outputs of the given `Command` and store them in an
     * `ActionResult`.
     */
    void captureAllOutputs(const Command &command, ActionResult *result) const;

    // It's illegal to copy a StagedDirectory since destroying one copy
    // would cause the other's local directory to be deleted.
    StagedDirectory(const StagedDirectory &) = delete;
    StagedDirectory &operator=(StagedDirectory const &) = delete;

    /*
     * Implementing the `captureAllOutputs()` algorithm in a generic way for
     * testing it in isolation. The callback functions that will capture files
     * and directories are tested separately.
     */
    typedef std::function<OutputFile(const char *path)> CaptureFileCallback;
    typedef std::function<OutputDirectory(const char *directory)>
        CaptureDirectoryCallback;

    void
    captureAllOutputs(const Command &command, ActionResult *result,
                      CaptureFileCallback captureFileFunction,
                      CaptureDirectoryCallback captureDirectoryFunction) const;

  protected:
    std::string d_path;

  private:
    static OutputFile
    captureFile(const std::string &outputFilename,
                const std::string &pathInInputRoot,
                StagedDirectory::CaptureFileCallback captureFileFunction);

    static OutputDirectory captureDirectory(
        const std::string &outputFilename, const std::string &pathInInputRoot,
        StagedDirectory::CaptureDirectoryCallback captureDirectoryFunction);

    // Helpers to normalize and assert that the paths in a Command are valid:
    static void assertNoInvalidSlashes(const std::string &path);
    static void assertPathInsideInputRoot(const std::string &pathFromRoot);
    static std::string getWorkingDirectory(const Command &command);
    static std::string pathInInputRoot(const std::string &name,
                                       const std::string &workingDirectory);

    // Given a path, calls `lstat()`.
    // If the path exists, writes its `st_mode` and returns `true`. Otherwise
    // returns `false`.
    // If `lstat()` fails with an error other than `ENOENT`, throws
    // `std::system_error`.
    static bool getStatMode(const std::string &path, mode_t *st_mode);
};

struct StagedDirectoryUtils {
    // These helpers allow to open files and directories while making sure that
    // they are located under the directory specified by `root_dir_fd` and
    // without following symlinks.

    // If successful, returns a file descriptor to the filed pointed to by
    // `path`. Otherwise, throws `std::system_error`.
    static int openFileInInputRoot(const int root_dir_fd,
                                   const std::string &relative_path);

    // If successful, returns a file descriptor to the last directory in
    // `path`. Otherwise, throws `std::system_error`.
    static int openDirectoryInInputRoot(const int root_dir_fd,
                                        const std::string &path);

    // Returns whether the path points to a regular file under the input root.
    static bool fileInInputRoot(const int root_dir_fd,
                                const std::string &path);

    // Returns whether the path points to a directory under the input root.
    static bool directoryInInputRoot(const int root_dir_fd,
                                     const std::string &path);
};

} // namespace buildboxcommon

#endif
