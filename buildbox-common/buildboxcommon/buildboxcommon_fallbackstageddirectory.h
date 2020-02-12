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
#include <functional>
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

    OutputFile captureFile(const char *relative_path,
                           const Command &command) const override;
    OutputDirectory captureDirectory(const char *relative_path,
                                     const Command &command) const override;

    /* Helpers marked as protected to unit test */

    // Given a relative path, uses `openAt()` to get a file descriptor to it,
    // resolving the path from the stage directory FD.
    // On errors throws an `std::system_error` exception.
    int openFile(const char *relative_path) const;

    OutputFile
    captureFile(const char *relative_path,
                const std::function<void(const int fd, const Digest &digest)>
                    &upload_file_function) const;

    OutputDirectory
    captureDirectory(const char *relative_path,
                     const std::function<Digest(const std::string &path)>
                         &upload_directory_function) const;

  protected:
    FallbackStagedDirectory();

    std::shared_ptr<Client> d_casClient;
    TemporaryDirectory d_stage_directory;

    int d_stage_directory_fd;

    OutputFile captureFile(const char *relative_path,
                           const char *workingDirectory) const;

    Digest uploadDirectory(const std::string &path) const;
};
} // namespace buildboxcommon

#endif
