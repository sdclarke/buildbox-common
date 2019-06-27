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

#include <buildboxcommon_requestmetadata.h>

#include <buildboxcommon_protos.h>

using namespace buildboxcommon;

const std::string RequestMetadataGenerator::HEADER_NAME =
    "requestmetadata-bin";

RequestMetadataGenerator::RequestMetadataGenerator() {}

RequestMetadataGenerator::RequestMetadataGenerator(
    const std::string &tool_name, const std::string &tool_version)

{
    this->set_tool_details(tool_name, tool_version);
}

void RequestMetadataGenerator::attach_request_metadata(
    grpc::ClientContext *context) const
{
    this->attach_request_metadata(context, d_action_id, d_tool_invocation_id,
                                  d_correlated_invocations_id);
}

void RequestMetadataGenerator::set_tool_details(
    const std::string &tool_name, const std::string &tool_version)
{
    d_tool_details.set_tool_name(tool_name);
    d_tool_details.set_tool_version(tool_version);
}

void RequestMetadataGenerator::set_action_id(const std::string &action_id)
{
    d_action_id = action_id;
}

void RequestMetadataGenerator::set_tool_invocation_id(
    const std::string &tool_invocation_id)
{
    d_tool_invocation_id = tool_invocation_id;
}

void RequestMetadataGenerator::set_correlated_invocations_id(
    const std::string &correlated_invocations_id)
{
    d_correlated_invocations_id = correlated_invocations_id;
}

RequestMetadata RequestMetadataGenerator::generate_request_metadata(
    const std::string &action_id, const std::string &tool_invocation_id,
    const std::string &correlated_invocations_id) const
{
    RequestMetadata metadata;
    metadata.mutable_tool_details()->CopyFrom(d_tool_details);

    metadata.set_action_id(action_id);
    metadata.set_tool_invocation_id(tool_invocation_id);
    metadata.set_correlated_invocations_id(correlated_invocations_id);

    return metadata;
}

void RequestMetadataGenerator::attach_request_metadata(
    grpc::ClientContext *context, const std::string &action_id,
    const std::string &tool_invocation_id,
    const std::string &correlated_invocations_id) const
{
    const RequestMetadata metadata = this->generate_request_metadata(
        action_id, tool_invocation_id, correlated_invocations_id);

    context->AddMetadata(HEADER_NAME, metadata.SerializeAsString());
}
