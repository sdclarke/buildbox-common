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

    virtual OutputFile captureFile(const char *relative_path,
                                   const Command &command) const = 0;

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

    void captureAllOutputs(
        const Command &command, ActionResult *result,
        CaptureFileCallback capture_file_function,
        CaptureDirectoryCallback capture_directory_function) const;

  protected:
    std::string d_path;
};
} // namespace buildboxcommon

#endif
