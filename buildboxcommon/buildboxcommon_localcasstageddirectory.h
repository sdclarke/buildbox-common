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

#ifndef BUILDBOXCOMMON_LOCALCASSTAGEDDIRECTORY_H
#define BUILDBOXCOMMON_LOCALCASSTAGEDDIRECTORY_H

#include <buildboxcommon_client.h>
#include <buildboxcommon_stageddirectory.h>

namespace buildboxcommon {

class LocalCasStagedDirectory final : public StagedDirectory {

  public:
    explicit LocalCasStagedDirectory(const Digest &digest,
                                     std::shared_ptr<Client> cas_client);

    /**
     * Close the connection to the remote and unstage.
     *
     * (Done by `~Client::StagedDirectory()`)
     */
    inline ~LocalCasStagedDirectory(){};

    OutputFile captureFile(const char *relative_path) const override;
    OutputDirectory captureDirectory(const char *relative_path) const override;

    // It's illegal to copy a LocalCasStagedDirectory since destroying one copy
    // would cause the other's local directory to be deleted.
    LocalCasStagedDirectory(const LocalCasStagedDirectory &) = delete;
    LocalCasStagedDirectory &
    operator=(LocalCasStagedDirectory const &) = delete;

  private:
    std::shared_ptr<Client> d_cas_client;
    std::unique_ptr<Client::StagedDirectory> d_cas_client_staged_directory;
};

} // namespace buildboxcommon

#endif // BUILDBOXCOMMON_LOCALCASSTAGEDDIRECTORY_H
