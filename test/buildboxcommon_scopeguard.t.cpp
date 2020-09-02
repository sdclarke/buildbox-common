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

#include <functional>

#include <gtest/gtest.h>

TEST(ScopeGuardTest, Guard)
{
    int numberOfInvocations = 0;
    {
        const auto f = [&numberOfInvocations]() { numberOfInvocations++; };
        buildboxcommon::ScopeGuard g(f);
    }
    ASSERT_EQ(numberOfInvocations, 1);
}

TEST(ScopeGuardTest, GuardCatchesStdException)
{
    int numberOfInvocations = 0;
    {
        const auto f = [&numberOfInvocations]() {
            numberOfInvocations++;
            throw std::runtime_error("Exception!");
        };

        buildboxcommon::ScopeGuard g(f);
    }

    ASSERT_EQ(numberOfInvocations, 1);
}

TEST(ScopeGuardTest, GuardCatchesAnyException)
{
    int numberOfInvocations = 0;
    {
        const auto f = [&numberOfInvocations]() {
            numberOfInvocations++;
            throw 123;
        };

        buildboxcommon::ScopeGuard g(f);
    }

    ASSERT_EQ(numberOfInvocations, 1);
}
