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

#include <buildboxcommon_stageddirectory.h>

namespace buildboxcommon {

void StagedDirectory::captureAllOutputs(const Command &command,
                                        ActionResult *result)
{
    for (const auto outputFilename : command.output_files()) {
        const std::string path =
            command.working_directory() + "/" + outputFilename;
        auto outputFile = this->captureFile(path.c_str());
        if (outputFile) {
            outputFile->set_path(outputFilename);
            *(result->add_output_files()) = *outputFile;
        }
    }
    for (const auto outputDirName : command.output_directories()) {
        const std::string path =
            command.working_directory() + "/" + outputDirName;
        auto outputDirectory = this->captureDirectory(path.c_str());
        if (outputDirectory) {
            outputDirectory->set_path(outputDirName);
            *(result->add_output_directories()) = *outputDirectory;
        }
    }
}

} // namespace buildboxcommon
