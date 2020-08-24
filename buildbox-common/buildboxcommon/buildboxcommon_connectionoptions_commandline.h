/*
 * Copyright 2020 Bloomberg Finance LP
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

#ifndef INCLUDED_BUILDBOXCOMMON_CONNECTIONOPTIONS_COMMANDLINE
#define INCLUDED_BUILDBOXCOMMON_CONNECTIONOPTIONS_COMMANDLINE

#include <buildboxcommon_commandline.h>

#include <string>
#include <vector>

namespace buildboxcommon {

class ConnectionOptions;

class ConnectionOptionsCommandLine {
  public:
    ConnectionOptionsCommandLine(const std::string &serviceName,
                                 const std::string &commandLinePrefix);

    static bool configureClient(const CommandLine &cml,
                                const std::string &prefix,
                                ConnectionOptions *client);

    const std::vector<buildboxcommon::CommandLineTypes::ArgumentSpec> &
    spec() const
    {
        return d_spec;
    }

  private:
    std::vector<buildboxcommon::CommandLineTypes::ArgumentSpec> d_spec;
};

} // namespace buildboxcommon

#endif
