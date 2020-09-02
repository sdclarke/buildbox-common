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

#ifndef INCLUDED_BUILDBOXCOMMON_SCOPEDGUARD
#define INCLUDED_BUILDBOXCOMMON_SCOPEDGUARD

#include <functional>

namespace buildboxcommon {

class ScopeGuard final {
  public:
    /* Take a function and invokes it when the object goes out of scope.
     *
     * Useful for when an action needs to be performed at the end of a
     * function regardless of exceptions or conditions that interrupt its
     * normal flow.
     *
     * To avoid terminating due to exceptions in a destructor, it will catch
     * and ignore exceptions thrown by the callback.
     */
    explicit ScopeGuard(const std::function<void()> &callback);
    ~ScopeGuard();

    // No copying allowed.
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(ScopeGuard const &) = delete;

  private:
    const std::function<void()> &d_callback;
};

} // namespace buildboxcommon

#endif
