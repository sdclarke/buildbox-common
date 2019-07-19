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

#include <buildboxcommon_localcasstageddirectory.h>

#include <buildboxcommon_client.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_protos.h>

using namespace buildboxcommon;

LocalCasStagedDirectory::LocalCasStagedDirectory(
    const Digest &digest, std::shared_ptr<Client> cas_client)
    : d_cas_client(cas_client),
      d_cas_client_staged_directory(cas_client->stage(digest))
{
    this->d_path = d_cas_client_staged_directory->path();
}

OutputFile
LocalCasStagedDirectory::captureFile(const char *relative_path) const
{
    return StagedDirectory::captureFile(relative_path, this->d_path.c_str(),
                                        this->d_cas_client);
}

OutputDirectory
LocalCasStagedDirectory::captureDirectory(const char *relative_path) const
{

    const std::string absolute_path =
        FileUtils::make_path_absolute(relative_path, this->d_path);

    const CaptureTreeResponse capture_response =
        this->d_cas_client->capture({absolute_path}, false);

    OutputDirectory captured_directory;
    captured_directory.set_path(relative_path);
    captured_directory.set_path(capture_response.responses(0).path());
    return captured_directory;
}
