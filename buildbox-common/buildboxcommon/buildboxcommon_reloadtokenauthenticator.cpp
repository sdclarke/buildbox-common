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

#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_reloadtokenauthenticator.h>
#include <buildboxcommon_stringutils.h>
#include <buildboxcommon_timeutils.h>

#include <chrono>
#include <grpcpp/channel.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace buildboxcommon {
ReloadTokenAuthenticator::ReloadTokenAuthenticator(
    const grpc::string &token_path, const char *refresh_time)
    : d_token_path(token_path)
{
    if (refresh_time == nullptr) {
        d_skip_refresh = true;
    }
    else {
        d_refresh_duration = ParseTime(refresh_time);
    }
    TrimAndSetToken();
}

grpc::Status ReloadTokenAuthenticator::GetMetadata(
    grpc::string_ref service_url, grpc::string_ref method_name,
    const grpc::AuthContext &,
    std::multimap<grpc::string, grpc::string> *metadata)
{
    if (!d_skip_refresh) {
        RefreshTokenIfNeeded();
    }
    const std::string tmp_token_string = GetTokenString();
    BUILDBOX_LOG_TRACE("Calling GetMetadata with args: ["
                       << service_url << " " << method_name
                       << "] and access token from path: [" << d_token_path
                       << "]");
    metadata->emplace("authorization", tmp_token_string);
    return grpc::Status::OK;
}

void ReloadTokenAuthenticator::RefreshTokenIfNeeded()
{
    const auto current_time = std::chrono::steady_clock::now();
    const auto next_refresh_time = GetNextRefreshTime();

    if (current_time >= next_refresh_time) {
        if (d_reload_lock.try_lock()) {
            std::lock_guard<std::mutex> reload_lock_guard(d_reload_lock,
                                                          std::adopt_lock);
            TrimAndSetToken();
        }
    }
}

void ReloadTokenAuthenticator::TrimAndSetToken()
{
    std::string new_token_string =
        FileUtils::getFileContents(d_token_path.c_str());
    // Trim the acccess token of any trailing
    // whitespace
    StringUtils::trim(&new_token_string);
    std::unique_lock<std::shared_timed_mutex> lock(d_token_string_lock);
    d_token_string = "Bearer " + new_token_string;
    BUILDBOX_LOG_INFO("Read and set access token from disk");

    // Set next reload time
    SetNextRefreshTimeFromNow();
}

std::chrono::seconds
ReloadTokenAuthenticator::ParseTime(const char *refresh_time_char) const
{
    const std::string refresh_time = std::string(refresh_time_char);
    if (refresh_time.empty()) {
        const std::string error_string =
            "Empty string cannot be specified for reload token interval";
        BUILDBOX_LOG_ERROR(error_string);
        BUILDBOXCOMMON_THROW_EXCEPTION(std::invalid_argument, error_string);
    }
    static const std::unordered_map<char, int> prefix_to_mult = {
        {'M', 60}, {'H', 60 * 60}};
    // toupper returns an int, so cast it to a char
    const char back = (char)toupper(refresh_time.back());
    int suffix = 0;
    int multiplier = 60;
    if (prefix_to_mult.find(back) != prefix_to_mult.end()) {
        multiplier = prefix_to_mult.at(back);
        suffix = 1;
    }
    std::size_t pos;
    int refresh_duration;
    const std::string error_string =
        "Invalid value specified for reload access time";
    try {
        refresh_duration = std::stoi(refresh_time, &pos) * multiplier;
        if (refresh_time.size() - suffix != pos) {
            BUILDBOXCOMMON_THROW_EXCEPTION(std::invalid_argument,
                                           error_string);
        }
    }
    catch (const std::invalid_argument &ia) {
        BUILDBOX_LOG_ERROR(error_string);
        throw ia;
    }
    return std::chrono::seconds(refresh_duration);
}

std::string ReloadTokenAuthenticator::GetTokenString()
{
    std::shared_lock<std::shared_timed_mutex> lock(d_token_string_lock);
    return d_token_string;
}

std::chrono::steady_clock::time_point
ReloadTokenAuthenticator::GetNextRefreshTime()
{
    std::shared_lock<std::shared_timed_mutex> lock(d_next_refresh_time_lock);
    return d_next_refresh_time;
}

void ReloadTokenAuthenticator::SetNextRefreshTimeFromNow()
{
    std::unique_lock<std::shared_timed_mutex> lock(d_next_refresh_time_lock);
    const auto now = std::chrono::steady_clock::now();
    d_next_refresh_time = now + d_refresh_duration;
}

} // namespace buildboxcommon
