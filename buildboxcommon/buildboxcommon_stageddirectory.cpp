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
#include <buildboxcommon_stageddirectory.h>

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace buildboxcommon;

OutputFile StagedDirectory::captureFile(const char *relative_path,
                                        const char *workingDirectory,
                                        std::shared_ptr<Client> cas_client)
{
    BUILDBOX_LOG_DEBUG("Uploading " << relative_path);
    const std::string file =
        workingDirectory + std::string("/") + relative_path;

    const int fd = open(file.c_str(), O_RDONLY);
    if (fd == -1) {
        if (errno == EACCES || errno == ENOENT) {
            return OutputFile();
        }
        throw std::system_error(errno, std::system_category());
    }
    if (FileUtils::is_directory(fd)) {
        close(fd);
        return OutputFile();
    }

    const Digest digest = CASHash::hash(fd);

    try {
        cas_client->upload(fd, digest);
        close(fd);
    }
    catch (...) {
        close(fd);
        throw;
    }

    OutputFile output_file;
    output_file.set_path(relative_path);
    output_file.mutable_digest()->CopyFrom(digest);
    output_file.set_is_executable(FileUtils::is_executable(file.c_str()));
    return output_file;
}

void StagedDirectory::captureAllOutputs(const Command &command,
                                        ActionResult *result) const
{
    CaptureFileCallback capture_file_function = [&](const char *path) {
        return this->captureFile(path);
    };

    CaptureDirectoryCallback capture_directory_function =
        [&](const char *path) { return this->captureDirectory(path); };

    return captureAllOutputs(command, result, capture_file_function,
                             capture_directory_function);
}

void StagedDirectory::captureAllOutputs(
    const Command &command, ActionResult *result,
    StagedDirectory::CaptureFileCallback capture_file_function,
    StagedDirectory::CaptureDirectoryCallback capture_directory_function) const
{
    for (const auto &outputFilename : command.output_files()) {
        const std::string path =
            command.working_directory() + "/" + outputFilename;

        OutputFile outputFile = capture_file_function(path.c_str());
        if (!outputFile.path().empty()) {
            outputFile.set_path(outputFilename);
            OutputFile *file_entry = result->add_output_files();
            file_entry->CopyFrom(outputFile);
        }
    }

    for (const auto &outputDirName : command.output_directories()) {
        const std::string path =
            command.working_directory() + "/" + outputDirName;

        OutputDirectory outputDirectory =
            capture_directory_function(path.c_str());
        if (!outputDirectory.path().empty()) {
            outputDirectory.set_path(outputDirName);
            OutputDirectory *directory_entry =
                result->add_output_directories();
            directory_entry->CopyFrom(outputDirectory);
        }
    }
}
