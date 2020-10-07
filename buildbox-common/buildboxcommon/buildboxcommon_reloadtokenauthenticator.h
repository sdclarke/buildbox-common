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

#ifndef INCLUDED_BUILDBOXCOMMON_RELOADTOKENAUTHENTICATOR
#define INCLUDED_BUILDBOXCOMMON_RELOADTOKENAUTHENTICATOR

#include <chrono>
#include <grpcpp/channel.h>
#include <grpcpp/security/credentials.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace buildboxcommon {
class ReloadTokenAuthenticator : public grpc::MetadataCredentialsPlugin {
  public:
    ReloadTokenAuthenticator(const grpc::string &token_path,
                             const char *refresh_time);

    grpc::Status
    GetMetadata(grpc::string_ref service_url, grpc::string_ref method_name,
                const grpc::AuthContext &channel_auth_context,
                std::multimap<grpc::string, grpc::string> *metadata) override;

    void RefreshTokenIfNeeded();

  private:
    void TrimAndSetToken();
    std::chrono::seconds ParseTime(const char *refresh_time_char) const;
    std::string GetTokenString();
    std::chrono::steady_clock::time_point GetNextRefreshTime();
    void SetNextRefreshTimeFromNow();

    std::mutex d_reload_lock;
    mutable std::shared_timed_mutex d_token_string_lock;
    mutable std::shared_timed_mutex d_next_refresh_time_lock;

    const std::string d_token_path;
    std::string d_token_string;
    std::chrono::steady_clock::time_point d_next_refresh_time;
    std::chrono::seconds d_refresh_duration;

    bool d_skip_refresh = false;
};

} // namespace buildboxcommon
#endif
