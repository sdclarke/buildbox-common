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
 * Represents a directory that has been "staged" by downloading it from CAS
 * onto the local filesystem.
 *
 * Currently, the only class that implements this is FallbackStagedDirectory,
 * but once the LocalCAS protocol has been finalized, a LocalCASStagedDirectory
 * will also be provided.
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

    /**
     * Upload the file at the given path (relative to the root of the
     * downloaded directory) to CAS and return an OutputFile.
     *
     * If there's no file at that path, or there's a directory at that path,
     * return a null pointer.
     */
    virtual std::shared_ptr<OutputFile>
    captureFile(const char *relativePath) = 0;

    /**
     * Upload the directory at the given path (relative to the root of the
     * download directory) to CAS and return an OutputDirectory.
     *
     * If there's no directory at that path, or there's a file at that path,
     * return a null pointer.
     */
    virtual std::shared_ptr<OutputDirectory>
    captureDirectory(const char *relativePath) = 0;

    /**
     * Capture all the outputs of the given Command and store them in the given
     * ActionResult.
     */
    virtual void captureAllOutputs(const Command &command,
                                   ActionResult *result);

    // It's illegal to copy a StagedDirectory since destroying one copy
    // would cause the other's local directory to be deleted.
    StagedDirectory(const StagedDirectory &) = delete;
    StagedDirectory &operator=(StagedDirectory const &) = delete;

  protected:
    std::string d_path;
};
} // namespace buildboxcommon

#endif
