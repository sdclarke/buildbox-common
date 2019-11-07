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

    // According to the REAPI, `Command.working_directory()` can be empty.
    // In that case, we want to avoid adding leading slashes to paths, which
    // would make them absolute.
    const std::string base_path = command.working_directory().empty()
                                      ? ""
                                      : command.working_directory() + "/";

    // Also:
    //  "The paths are relative to the working directory of the action
    //   execution. [...] The path MUST NOT include a trailing slash,
    //   nor a leading slash, being a relative path."
    auto assert_no_invalid_slashes = [](const std::string &path) {
        if (path.front() == '/' || path.back() == '/') {
            const auto error_message = "Output path in `Command` has "
                                       "leading or trailing slashes: \"" +
                                       path + "\"";
            BUILDBOX_LOG_ERROR(error_message);
            throw std::invalid_argument(error_message);
        }
    };

    for (const auto &outputFilename : command.output_files()) {
        assert_no_invalid_slashes(outputFilename);

        const std::string path = base_path + outputFilename;

        OutputFile outputFile = capture_file_function(path.c_str());
        if (!outputFile.path().empty()) {
            outputFile.set_path(outputFilename);
            OutputFile *file_entry = result->add_output_files();
            file_entry->CopyFrom(outputFile);
        }
    }

    for (const auto &outputDirName : command.output_directories()) {
        assert_no_invalid_slashes(outputDirName);

        const std::string path = base_path + outputDirName;

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
