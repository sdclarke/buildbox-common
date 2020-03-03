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

#include <buildboxcommonmetrics_udpwriter.h>
#include <gtest/gtest.h>

using namespace buildboxcommon::buildboxcommonmetrics;

TEST(MetricsTest, UdpWriterTestLocalhost)
{
    EXPECT_NO_THROW(UDPWriter(8125, "localhost"));
}

TEST(MetricsTest, UdpWriterTestLocalIP)
{
    EXPECT_NO_THROW(UDPWriter(8125, "127.0.0.1"));
}

TEST(MetricsTest, UdpWriterTestDefault) { EXPECT_NO_THROW(UDPWriter(8125)); }

TEST(MetricsTest, UdpWriterTestUnknownHost)
{
    const std::string fakehost = "thishostshouldnotexist";
    EXPECT_THROW(UDPWriter(8125, fakehost), std::runtime_error);
}
