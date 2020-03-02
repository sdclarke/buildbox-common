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

#ifndef BUILDBOXCOMMONMETRICS_METRICSCONFIGURATOR_H
#define BUILDBOXCOMMONMETRICS_METRICSCONFIGURATOR_H

#include <iostream>

#include <buildboxcommonmetrics_metriccollectorfactory.h>
#include <buildboxcommonmetrics_metricsconfigtype.h>

#include <buildboxcommonmetrics_countingmetricvalue.h>
#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_totaldurationmetricvalue.h>

namespace buildboxcommon {

namespace buildboxcommonmetrics {

class MetricsConfigurator {
  public:
    // Return a MetricsConfigType type
    // If publishing interval isn't set, defaults to 15 seconds.
    // This value is only used if PeriodicPublisherDaemon is used.
    static MetricsConfigType
    createMetricsConfig(const std::string &file, const std::string &udp_server,
                        const bool enable,
                        const size_t interval =
                            buildboxcommonmetrics::DEFAULT_PUBLISH_INTERVAL);

    // Alias template-to-Template type of the given PublisherType that
    // receives (from the MetricCollectorFactory) and submits the given
    // ValueTypes.
    // Usage with `typedef`:
    //    #typedef publisherTypeOf<MyPublisherType, ValueType1, ValueType2....>
    //    MyPublisher
    // Usage with `using`:
    //    using MyPublisher = publisherTypeOf<MyPublisherType, ValueType1,
    //    ValueType2....>
    template <template <class...> class PublisherType, class... ValueTypes>
    using publisherTypeOfValueTypes = PublisherType<ValueTypes...>;

    // Alias Template to generate the Template type of the given PublisherType
    // that receives (from the MetricCollectorFactory) and submits all
    // ValueTypes.
    // Usage with `typedef`:
    //    #typedef allMetricTypesPublisherType<MyPublisherType> MyPublisher
    // Usage with `using`:
    //    using MyPublisher = allMetricTypesPublisherType<MyPublisherType>
    template <template <class...> class PublisherType>
    using publisherTypeOfAllValueTypes = publisherTypeOfValueTypes<
        PublisherType, buildboxcommonmetrics::CountingMetricValue,
        buildboxcommonmetrics::DurationMetricValue,
        buildboxcommonmetrics::TotalDurationMetricValue>;

    // Sets-up the MetricCollectorFactory and creates a PublisherType
    // with the config given.
    template <class PublisherType>
    static std::shared_ptr<PublisherType>
    createMetricsPublisherWithConfig(const MetricsConfigType &config)
    {
        // Apply settings to MetricCollectorFactory
        MetricCollectorFactory::getInstance()->setMetricsEnabled(
            config.enable());

        // Create and return the requested publisher with the
        // given config
        return PublisherType::fromConfig(config);
    }

    // DEPRECATED
    // The following convenience methods were moved to `metricsconfigutil`
    // and will be permanently removed from `metricsconfigurator` in the near
    // future. They kept here as alias functions for backwards compatibility.
    static bool isMetricsOption(const std::string &option);
    static void metricsParser(const std::string &argument_name,
                              const std::string &value,
                              MetricsConfigType *config);
    static void usage(std::ostream &out = std::clog);
    static void parseHostPortString(const std::string &inputString,
                                    std::string *serverRet, uint16_t *portRet);
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif // BUILDBOXCOMMONMETRICS_METICSCONFIGURATOR_H
