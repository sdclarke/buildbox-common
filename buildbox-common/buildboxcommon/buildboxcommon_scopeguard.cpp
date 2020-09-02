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

#include <buildboxcommon_scopeguard.h>

#include <buildboxcommon_logging.h>

namespace buildboxcommon {

ScopeGuard::ScopeGuard(const std::function<void()> &callback)
    : d_callback(callback)
{
}

ScopeGuard::~ScopeGuard()
{
    try {
        d_callback();
        BUILDBOX_LOG_DEBUG("Callback function returned successfully")
    }
    catch (const std::exception &e) {
        BUILDBOX_LOG_WARNING(
            "Callback function threw an exception: " << e.what());
    }
    catch (...) {
        BUILDBOX_LOG_WARNING("Callback function threw");
    }
}

} // namespace buildboxcommon
