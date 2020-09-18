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

#include <buildboxcommon_stringutils.h>

#include <algorithm>
#include <cctype>

namespace buildboxcommon {

void StringUtils::ltrim(std::string *s)
{
    s->erase(s->begin(),
             std::find_if(s->begin(), s->end(),
                          [](unsigned char ch) { return !std::isspace(ch); }));
}

void StringUtils::ltrim(std::string *s, const std::function<int(int)> &filter)
{
    s->erase(s->begin(),
             std::find_if(s->begin(), s->end(), [&filter](unsigned char ch) {
                 return !filter(ch);
             }));
}

void StringUtils::rtrim(std::string *s)
{
    s->erase(std::find_if(s->rbegin(), s->rend(),
                          [](unsigned char ch) { return !std::isspace(ch); })
                 .base(),
             s->end());
}

void StringUtils::rtrim(std::string *s, const std::function<int(int)> &filter)
{
    s->erase(std::find_if(s->rbegin(), s->rend(),
                          [&filter](unsigned char ch) { return !filter(ch); })
                 .base(),
             s->end());
}

void StringUtils::trim(std::string *s)
{
    ltrim(s);
    rtrim(s);
}

void StringUtils::trim(std::string *s, const std::function<int(int)> &filter)
{
    ltrim(s, filter);
    rtrim(s, filter);
}

std::string StringUtils::ltrim(const std::string &s)
{
    std::string copy(s);
    ltrim(&copy);
    return copy;
}

std::string StringUtils::ltrim(const std::string &s,
                               const std::function<int(int)> &filter)
{
    std::string copy(s);
    ltrim(&copy, filter);
    return copy;
}

std::string StringUtils::rtrim(const std::string &s)
{
    std::string copy(s);
    rtrim(&copy);
    return copy;
}

std::string StringUtils::rtrim(const std::string &s,
                               const std::function<int(int)> &filter)
{
    std::string copy(s);
    rtrim(&copy, filter);
    return copy;
}

std::string StringUtils::trim(const std::string &s)
{
    std::string copy(s);
    trim(&copy);
    return copy;
}

std::string StringUtils::trim(const std::string &s,
                              const std::function<int(int)> &filter)
{
    std::string copy(s);
    trim(&copy, filter);
    return copy;
}

} // namespace buildboxcommon
