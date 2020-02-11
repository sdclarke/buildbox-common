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

#ifndef INCLUDED_BUILDBOXCOMMONMETRICS_SCOPEDPERIODICPUBLISHERDAEMON_H
#define INCLUDED_BUILDBOXCOMMONMETRICS_SCOPEDPERIODICPUBLISHERDAEMON_H

#include <atomic>
#include <thread>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

template <class PublisherType> class ScopedPeriodicPublisherDaemon {
  private:
    std::atomic<bool> d_shutDown;
    const size_t d_publishIntervalSeconds;
    std::thread d_publisherThread;
    PublisherType d_publisher;

    void run()
    {
        while (!d_shutDown) {
            d_publisher.publish();
            std::this_thread::sleep_for(
                std::chrono::seconds(d_publishIntervalSeconds));
        }
    }

    void startPublisherThread()
    {
        d_publisherThread =
            std::thread(&ScopedPeriodicPublisherDaemon::run, this);
    }

  public:
    explicit ScopedPeriodicPublisherDaemon(const bool enabled,
                                           const size_t publishIntervalSeconds)
        : ScopedPeriodicPublisherDaemon(enabled, publishIntervalSeconds,
                                        PublisherType())
    {
    }

    explicit ScopedPeriodicPublisherDaemon(const bool enabled,
                                           const size_t publishIntervalSeconds,
                                           const PublisherType &publisher)
        : d_publisher(publisher), d_shutDown(false),
          d_publishIntervalSeconds(publishIntervalSeconds)
    {
        if (enabled) {
            startPublisherThread();
        }
    }

    ~ScopedPeriodicPublisherDaemon() { stop(); }

    void stop()
    {
        d_shutDown = true;
        if (d_publisherThread.joinable()) {
            d_publisherThread.join();
        }
    }
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif
