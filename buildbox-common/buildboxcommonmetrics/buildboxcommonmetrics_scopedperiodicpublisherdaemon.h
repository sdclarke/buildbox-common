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

#include <condition_variable>
#include <thread>

namespace buildboxcommon {
namespace buildboxcommonmetrics {

template <class PublisherType> class ScopedPeriodicPublisherDaemon {
  private:
    std::condition_variable d_shutDownCondition;
    std::mutex d_shutDownMutex;
    bool d_shutDown;

    bool d_enabled;
    const std::chrono::seconds d_publishIntervalSeconds;
    std::thread d_publisherThread;
    PublisherType d_publisher;

    void run()
    {
        while (true) {
            // Blocking while waiting `d_publishIntervalSeconds` to elapse or
            // `d_shutDown` to be set...
            std::unique_lock<std::mutex> shutDownLock(d_shutDownMutex);
            const bool shutdown_requested = d_shutDownCondition.wait_for(
                shutDownLock, d_publishIntervalSeconds,
                [this]() { return this->d_shutDown; });

            if (shutdown_requested) {
                // We were signaled to stop: exit the thread immediately.
                // (`~ScopedPeriodicPublisherDaemon()` will carry out the last
                // publication.)
                return;
            }

            d_publisher.publish();
        }
    }

    void startPublisherThread()
    {
        if (!d_enabled) {
            return;
        }

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
        : d_enabled(enabled), d_publishIntervalSeconds(publishIntervalSeconds),
          d_publisher(publisher), d_shutDown(false)
    {
        startPublisherThread();
    }

    ~ScopedPeriodicPublisherDaemon()
    {
        if (!d_enabled) {
            return;
        }

        stop();
        d_publisher.publish();
    }

    void stop()
    {
        if (!d_enabled) {
            return;
        }

        // Signaling the publisher thread and waiting for it to exit:
        {
            const std::lock_guard<std::mutex> lock(d_shutDownMutex);
            d_shutDown = true;
        }
        d_shutDownCondition.notify_one();

        if (d_publisherThread.joinable()) {
            d_publisherThread.join();
        }
    }
};

} // namespace buildboxcommonmetrics
} // namespace buildboxcommon

#endif
