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

#include <buildboxcommon_connectionoptions_commandline.h>

#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_logging.h>

namespace buildboxcommon {

using ArgumentSpec = buildboxcommon::CommandLineTypes::ArgumentSpec;
using DataType = buildboxcommon::CommandLineTypes::DataType;
using TypeInfo = buildboxcommon::CommandLineTypes::TypeInfo;
using DefaultValue = buildboxcommon::CommandLineTypes::DefaultValue;

ConnectionOptionsCommandLine::ConnectionOptionsCommandLine(
    const std::string &serviceName, const std::string &commandLinePrefix)
{
    d_spec.emplace_back(commandLinePrefix + "remote",
                        "URL for the " + serviceName + " service",
                        TypeInfo(DataType::COMMANDLINE_DT_STRING),
                        ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG);
    d_spec.emplace_back(commandLinePrefix + "instance",
                        "Name of the " + serviceName + " instance",
                        TypeInfo(DataType::COMMANDLINE_DT_STRING),
                        ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG,
                        DefaultValue(""));
    d_spec.emplace_back(commandLinePrefix + "server-cert",
                        "Public server certificate for " + serviceName +
                            " TLS (PEM-encoded)",
                        TypeInfo(DataType::COMMANDLINE_DT_STRING),
                        ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    d_spec.emplace_back(commandLinePrefix + "client-key",
                        "Private client key for " + serviceName +
                            " TLS (PEM-encoded)",
                        TypeInfo(DataType::COMMANDLINE_DT_STRING),
                        ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    d_spec.emplace_back(commandLinePrefix + "client-cert",
                        "Private client certificate for " + serviceName +
                            " TLS (PEM-encoded)",
                        TypeInfo(DataType::COMMANDLINE_DT_STRING),
                        ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    d_spec.emplace_back(
        commandLinePrefix + "access-token",
        "Access Token for authentication " + serviceName +
            " (e.g. JWT, OAuth access token, etc), "
            "will be included as an HTTP Authorization bearer token",
        TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL,
        ArgumentSpec::C_WITH_ARG);
    d_spec.emplace_back(commandLinePrefix + "googleapi-auth",
                        "Use GoogleAPIAuth for " + serviceName + " service",
                        TypeInfo(DataType::COMMANDLINE_DT_BOOL),
                        ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG,
                        DefaultValue(false));
    d_spec.emplace_back(commandLinePrefix + "retry-limit",
                        "Number of times to retry on grpc errors for " +
                            serviceName + " service",
                        TypeInfo(DataType::COMMANDLINE_DT_STRING),
                        ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG,
                        DefaultValue("4"));
    d_spec.emplace_back(commandLinePrefix + "retry-delay",
                        "How long to wait in milliseconds before the first "
                        "grpc retry for " +
                            serviceName + " service",
                        TypeInfo(DataType::COMMANDLINE_DT_STRING),
                        ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG,
                        DefaultValue("1000"));
}

bool ConnectionOptionsCommandLine::configureClient(
    const CommandLine &cml, const std::string &commandLinePrefix,
    ConnectionOptions *client)
{
    if (client == nullptr) {
        BUILDBOX_LOG_ERROR("invalid argument: 'client' is nullptr");
        return false;
    }

    std::string optionName = commandLinePrefix + "remote";
    client->d_url =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    optionName = commandLinePrefix + "instance";
    client->d_instanceName =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    optionName = commandLinePrefix + "server-cert";
    client->d_serverCertPath =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    optionName = commandLinePrefix + "client-key";
    client->d_clientKeyPath =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    optionName = commandLinePrefix + "client-cert";
    client->d_clientCertPath =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    optionName = commandLinePrefix + "access-token";
    client->d_accessTokenPath =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    optionName = commandLinePrefix + "googleapi-auth";
    client->d_useGoogleApiAuth =
        cml.exists(optionName) ? cml.getBool(optionName) : false;

    optionName = commandLinePrefix + "retry-limit";
    client->d_retryLimit =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    optionName = commandLinePrefix + "retry-delay";
    client->d_retryDelay =
        cml.exists(optionName) ? cml.getString(optionName).c_str() : nullptr;

    return true;
}

} // namespace buildboxcommon
