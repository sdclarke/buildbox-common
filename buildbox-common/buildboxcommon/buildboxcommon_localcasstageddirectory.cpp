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
#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_protos.h>
#include <buildboxcommon_timeutils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace buildboxcommon;

LocalCasStagedDirectory::LocalCasStagedDirectory(
    const Digest &digest, const std::string &path,
    std::shared_ptr<Client> cas_client)
    : d_cas_client(cas_client),
      d_cas_client_staged_directory(cas_client->stage(digest, path))
{
    this->d_path = d_cas_client_staged_directory->path();

    this->d_staged_directory_fd = open(
        d_cas_client_staged_directory->path().c_str(), O_DIRECTORY | O_RDONLY);

    if (d_staged_directory_fd == -1) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error opening staged directory: \"" << d_path << "\"");
    }
}

LocalCasStagedDirectory::~LocalCasStagedDirectory()
{
    close(d_staged_directory_fd);
}

OutputFile LocalCasStagedDirectory::captureFile(const char *relative_path,
                                                const Command &command) const
{
    if (!StagedDirectoryUtils::fileInInputRoot(this->d_staged_directory_fd,
                                               relative_path)) {
        return OutputFile();
    }

    const std::vector<std::string> properties(
        command.output_node_properties().cbegin(),
        command.output_node_properties().cend());

    const std::string absolute_path =
        FileUtils::makePathAbsolute(relative_path, this->d_path);

    const CaptureFilesResponse response =
        this->d_cas_client->captureFiles({absolute_path}, properties, false);

    if (response.responses().empty()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Error capturing \""
                                    << absolute_path
                                    << "\": server returned empty response");
    }

    const auto captured_file = response.responses(0);

    if (captured_file.status().code() != grpc::StatusCode::OK) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Error capturing \""
                                    << absolute_path << "\": "
                                    << captured_file.status().message());
    }

    OutputFile output_file;
    output_file.set_path(relative_path);
    output_file.mutable_digest()->CopyFrom(captured_file.digest());
    output_file.set_is_executable(
        FileUtils::isExecutable(absolute_path.c_str()));
    *output_file.mutable_node_properties() = captured_file.node_properties();
    return output_file;
}

OutputDirectory
LocalCasStagedDirectory::captureDirectory(const char *relative_path,
                                          const Command &command) const
{
    if (!StagedDirectoryUtils::directoryInInputRoot(
            this->d_staged_directory_fd, relative_path)) {
        // If the directory does not exist or is outside the input root, we
        // just ignore it:
        return OutputDirectory();
    }

    const std::string absolute_path =
        FileUtils::makePathAbsolute(relative_path, this->d_path);

    const std::vector<std::string> properties(
        command.output_node_properties().cbegin(),
        command.output_node_properties().cend());

    const CaptureTreeResponse capture_response =
        this->d_cas_client->captureTree({absolute_path}, properties, false);

    OutputDirectory captured_directory;
    captured_directory.set_path(relative_path);
    captured_directory.mutable_tree_digest()->CopyFrom(
        capture_response.responses(0).tree_digest());
    return captured_directory;
}
