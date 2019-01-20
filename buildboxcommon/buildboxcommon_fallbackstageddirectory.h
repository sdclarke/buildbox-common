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
     * Download the directory with the given digest from CAS.
     */
    FallbackStagedDirectory(const Digest &digest,
                            std::shared_ptr<Client> casClient);

    ~FallbackStagedDirectory() override;
    std::shared_ptr<OutputFile> captureFile(const char *relativePath) override;
    std::shared_ptr<OutputDirectory>
    captureDirectory(const char *relativePath) override;

  private:
    void downloadDirectory(const Digest &digest, const char *path);
    void downloadFile(const Digest &digest, bool executable, const char *path);
    Directory uploadDirectoryRecursively(Tree *tree, DIR *dirStream,
                                         const char *relativePath);

    std::shared_ptr<Client> d_casClient;
};
} // namespace buildboxcommon

#endif
