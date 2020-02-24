// Copyright 2020 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <buildboxcommon_exception.h>
#include <buildboxcommonmetrics_statsdpublishercreator.h>

namespace buildboxcommon {

namespace buildboxcommonmetrics {

std::shared_ptr<StatsDPublisherType>
StatsdPublisherCreator::createStatsdPublisher(
    const MetricsConfigType &metricsConfig)
{
    buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod
        publishMethod = buildboxcommonmetrics::StatsDPublisherOptions::
            PublishMethod::StdErr;

    std::string publishPath = "";
    uint16_t publishPort = 0;

    if (!metricsConfig.udp_server().empty()) {
        publishMethod =
            buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::UDP;
        parseHostPortString(metricsConfig.udp_server(), &publishPath,
                            &publishPort);
    }
    else if (!metricsConfig.file().empty()) {
        publishMethod =
            buildboxcommonmetrics::StatsDPublisherOptions::PublishMethod::File;
        publishPath = metricsConfig.file();
    }

    return std::make_shared<StatsDPublisherType>(
        StatsDPublisherType(publishMethod, publishPath, publishPort));
}

void StatsdPublisherCreator::parseHostPortString(
    const std::string &inputString, std::string *serverRet, uint16_t *portRet)
{
    if (serverRet == nullptr || portRet == nullptr) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::invalid_argument,
            "null pointer detected in output parameters");
    }

    // NOTE: This only works for IPv4 addresses, not IPv6.
    const std::size_t split_at = inputString.find_last_of(":");
    if (split_at != std::string::npos &&
        split_at + 2 <
            inputString.size()) { // e.g. `localhost:1` or `example.org:1`
        try {
            const uint32_t tmp =
                (uint32_t)std::stoul(inputString.substr(split_at + 1));
            if (tmp > std::numeric_limits<uint16_t>::max()) {
                BUILDBOXCOMMON_THROW_EXCEPTION(
                    std::range_error,
                    "Invalid port specified (value too large): '" +
                        inputString.substr(split_at + 1) + "'");
            }
            *portRet = (uint16_t)tmp;
            *serverRet =
                inputString.substr(0, std::min(split_at, inputString.size()));
        }
        catch (const std::invalid_argument &) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::invalid_argument,
                "Invalid port specified (cannot be parsed to int): '" +
                    inputString.substr(split_at + 1) + "'");
        }
    }
    else { // e.g. `localhost` or `localhost:`
        // default to port 8125 if not specified in the string
        *portRet = 8125;
        *serverRet =
            inputString.substr(0, std::min(split_at, inputString.size()));
    }
}

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
