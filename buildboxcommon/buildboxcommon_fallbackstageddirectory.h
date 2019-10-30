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

#ifndef INCLUDED_BUILDBOXCOMMON_FALLBACKSTAGEDDIRECTORY
#define INCLUDED_BUILDBOXCOMMON_FALLBACKSTAGEDDIRECTORY

#include <buildboxcommon_client.h>
#include <buildboxcommon_stageddirectory.h>
#include <buildboxcommon_temporarydirectory.h>

#include <dirent.h>
#include <memory>

namespace buildboxcommon {
/**
 * An implementation of StagedDirectory that uses only the base CAS protocol
 * from the Remote Execution API specification.
 */
class FallbackStagedDirectory : public StagedDirectory {
  public:
    /**
     * Download the directory with the given digest from CAS, to location on
     * disk specified by path.
     */
    FallbackStagedDirectory(const Digest &digest, const std::string &path,
                            std::shared_ptr<Client> cas_client);

    ~FallbackStagedDirectory() override;

    OutputFile captureFile(const char *relative_path) const override;
    OutputDirectory captureDirectory(const char *relative_path) const override;

  private:
    void downloadDirectory(const Digest &digest, const char *path) const;
    void downloadFile(const Digest &digest, bool executable,
                      const char *path) const;

    /**
     * will close the file descriptor that is passed in
     */
    Directory uploadDirectoryRecursively(Tree *tree, const int dirFD) const;

    /**
     * Helper function to capture directory using file descriptor.
     * Will not close file descriptor that is passed in.
     */
    OutputFile captureFileWithFD(const int dirFD,
                                 const char *relative_path) const;
    std::shared_ptr<Client> d_casClient;
    TemporaryDirectory d_stage_directory;

    OutputFile captureFile(const char *relative_path,
                           const char *workingDirectory) const;
};
} // namespace buildboxcommon

#endif
