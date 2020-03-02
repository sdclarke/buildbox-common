// Copyright 2019 Bloomberg Finance L.P
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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_STATSDPUBLISHER_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_STATSDPUBLISHER_H

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <time.h>
#include <unordered_map>
#include <vector>

#include <buildboxcommonmetrics_metriccollectorfactory.h>

#include <buildboxcommonmetrics_metricsconfigtype.h>
#include <buildboxcommonmetrics_metricsconfigutil.h>

#include <buildboxcommonmetrics_filewriter.h>
#include <buildboxcommonmetrics_statsdpublisheroptions.h>
#include <buildboxcommonmetrics_udpwriter.h>

namespace buildboxcommon {
namespace buildboxcommonmetrics {
/**
 * StatsDPublisher
 */
template <class... ValueTypeList> class StatsDPublisher {
  public:
    // Constructors
    // Specific Constructor
    //  Use given MetricCollectorFactory and PublishMethod/PublishPath
    explicit StatsDPublisher(
        const StatsDPublisherOptions::PublishMethod &publishMethod =
            StatsDPublisherOptions::PublishMethod::StdErr,
        const std::string &publishPath = "", int publishPort = 0,
        MetricCollectorFactory *metricCollectorFactory =
            MetricCollectorFactory::getInstance())
        : d_metricCollectorFactory(metricCollectorFactory),
          d_publishMethod(publishMethod), d_publishPath(publishPath),
          d_publishPort(publishPort)
    {
        switch (d_publishMethod) {
            case StatsDPublisherOptions::PublishMethod::File: {
                if (publishPath.size() == 0) {
                    throw std::runtime_error(
                        "StatsD Publish Method set to `File` but `filePath` "
                        "provided is empty.");
                }
            } break;
            case StatsDPublisherOptions::PublishMethod::UDP: {
                if (publishPath.size() == 0 || publishPort <= 0) {
                    throw std::runtime_error(
                        "StatsD Publish Method set to `UDP` but `path=[" +
                        publishPath + "]`, `port=[" +
                        std::to_string(publishPort) + "]`");
                }
            } break;
            case StatsDPublisherOptions::PublishMethod::StdErr:
                // No special checks here
                break;
        }
    }

    // Construct a StatsDPublisher with the given config
    static std::shared_ptr<StatsDPublisher>
    fromConfig(const MetricsConfigType &metricsConfig)
    {
        auto publishMethod = buildboxcommonmetrics::StatsDPublisherOptions::
            PublishMethod::StdErr;

        std::string publishPath = "";
        uint16_t publishPort = 0;

        if (!metricsConfig.udp_server().empty()) {
            publishMethod = buildboxcommonmetrics::StatsDPublisherOptions::
                PublishMethod::UDP;
            buildboxcommonmetrics::MetricsConfigUtil::parseHostPortString(
                metricsConfig.udp_server(), &publishPath, &publishPort);
        }
        else if (!metricsConfig.file().empty()) {
            publishMethod = buildboxcommonmetrics::StatsDPublisherOptions::
                PublishMethod::File;
            publishPath = metricsConfig.file();
        }

        return std::make_shared<StatsDPublisher>(
            StatsDPublisher(publishMethod, publishPath, publishPort));
    };

    void publish()
    {
        std::vector<std::string> statsDMetrics;
        gatherStatsDFromNextCollectors<ValueTypeList...>(&statsDMetrics);

        // Figure out config
        switch (d_publishMethod) {
            case StatsDPublisherOptions::PublishMethod::StdErr: {
                const auto now = std::chrono::system_clock::now();

                struct tm localtime;
                const auto now_time_t =
                    std::chrono::system_clock::to_time_t(now);
                localtime_r(&now_time_t, &localtime);

                const auto now_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) %
                    1000;

                std::cerr << std::put_time(&localtime, "%FT%T") << '.'
                          << std::setfill('0') << std::setw(3)
                          << now_ms.count() << std::put_time(&localtime, "%z")
                          << " buildbox Metrics:\n";

                for (const std::string &metric : statsDMetrics) {
                    std::cerr << metric << "\n";
                }
            } break;
            case StatsDPublisherOptions::PublishMethod::File: {
                FileWriter fileWriter(d_publishPath);
                for (const std::string &metric : statsDMetrics) {
                    fileWriter.write(metric + "\n");
                }
            } break;
            case StatsDPublisherOptions::PublishMethod::UDP: {
                UDPWriter udpWriter(d_publishPort, d_publishPath);
                for (const std::string &metric : statsDMetrics) {
                    udpWriter.write(metric + "\n");
                }
            } break;
        }
    }

    std::string publishPath() const { return d_publishPath; }
    StatsDPublisherOptions::PublishMethod publishMethod() const
    {
        return d_publishMethod;
    }
    int publishPort() const { return d_publishPort; }

  private:
    MetricCollectorFactory *d_metricCollectorFactory;
    StatsDPublisherOptions::PublishMethod d_publishMethod;
    std::string d_publishPath;
    int d_publishPort;

    // For a single ValueType, store all metrics from beginning to end of
    // iterable
    template <class ValueType>
    void
    gatherStatsDFromValueTypeCollector(MetricCollector<ValueType> *collector,
                                       std::vector<std::string> *statsDMetrics)
    {
        const auto snapshot = collector->getSnapshot();
        for (const auto &entry : snapshot) {
            const std::string &metricName = entry.first;
            const ValueType &metricValue = entry.second;
            statsDMetrics->emplace_back(metricValue.toStatsD(metricName));
        }
    }

    // Template Functions that call gatherStatsDFromValueTypeCollector
    //                              for everything in the Parameter Pack
    // Those are recursively expanded by the compiler
    // 1 Template parameter
    template <class ValueType>
    void
    gatherStatsDFromNextCollectors(std::vector<std::string> *statsDMetrics)
    {
        gatherStatsDFromValueTypeCollector<ValueType>(
            d_metricCollectorFactory->getCollector<ValueType>(),
            statsDMetrics);
    }
    // >= 2 Template parameters
    template <class ValueType, class NextValueType, class... Rest>
    void
    gatherStatsDFromNextCollectors(std::vector<std::string> *statsDMetrics)
    {
        gatherStatsDFromValueTypeCollector<ValueType>(
            d_metricCollectorFactory->getCollector<ValueType>(),
            statsDMetrics);

        gatherStatsDFromNextCollectors<NextValueType, Rest...>(statsDMetrics);
    }
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon
#endif
