/*
 * Copyright 2019 Bloomberg Finance LP
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

#include <buildboxcommon_protos.h>
#include <buildboxcommon_requestmetadata.h>

#include <gtest/gtest.h>

using namespace buildboxcommon;

const std::string tool_name = "testing-tool";
const std::string tool_version = "v1.2.3";

class EmptyRequestMetadataFixture : public RequestMetadataGenerator,
                                    public ::testing::Test {
  protected:
    EmptyRequestMetadataFixture() : RequestMetadataGenerator() {}
};

TEST_F(EmptyRequestMetadataFixture, TestDefaultConstructor)
{
    ASSERT_TRUE(this->d_tool_details.tool_name().empty());
    ASSERT_TRUE(this->d_tool_details.tool_version().empty());
    ASSERT_TRUE(this->d_action_id.empty());
    ASSERT_TRUE(this->d_tool_invocation_id.empty());
    ASSERT_TRUE(this->d_correlated_invocations_id.empty());
}

class RequestMetadataFixture : public RequestMetadataGenerator,
                               public ::testing::Test {
  protected:
    RequestMetadataFixture()
        : RequestMetadataGenerator(tool_name, tool_version)
    {
    }
};

TEST_F(RequestMetadataFixture, TestToolDetailsConstructor)
{
    ASSERT_EQ(this->d_tool_details.tool_name(), tool_name);
    ASSERT_EQ(this->d_tool_details.tool_version(), tool_version);

    ASSERT_TRUE(this->d_action_id.empty());
    ASSERT_TRUE(this->d_tool_invocation_id.empty());
    ASSERT_TRUE(this->d_correlated_invocations_id.empty());
}

TEST_F(RequestMetadataFixture, RequestMetadataKey)
{
    ASSERT_EQ(this->HEADER_NAME, "requestmetadata-bin");
}

TEST_F(RequestMetadataFixture, TestSetters)
{
    this->set_tool_details("new testing tool", "0.1");
    this->set_action_id("action1");
    this->set_tool_invocation_id("invocation2");
    this->set_correlated_invocations_id("correlation3");

    ASSERT_EQ(this->d_tool_details.tool_name(), "new testing tool");
    ASSERT_EQ(this->d_tool_details.tool_version(), "0.1");
    ASSERT_EQ(this->d_action_id, "action1");
    ASSERT_EQ(this->d_tool_invocation_id, "invocation2");
    ASSERT_EQ(this->d_correlated_invocations_id, "correlation3");
}

TEST_F(RequestMetadataFixture, TestGeneratedMetadata)
{
    const std::string action_id = "action-alpha";
    const std::string tool_invocation_id = "invocation-india";
    const std::string correlated_invocations_id = "correlated-charlie";

    const RequestMetadata metadata = this->generate_request_metadata(
        action_id, tool_invocation_id, correlated_invocations_id);

    ASSERT_EQ(metadata.tool_details().tool_name(), tool_name);
    ASSERT_EQ(metadata.tool_details().tool_version(), tool_version);

    ASSERT_EQ(metadata.action_id(), action_id);
    ASSERT_EQ(metadata.tool_invocation_id(), tool_invocation_id);
    ASSERT_EQ(metadata.correlated_invocations_id(), correlated_invocations_id);
}
