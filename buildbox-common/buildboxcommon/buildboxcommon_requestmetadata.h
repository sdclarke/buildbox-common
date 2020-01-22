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

#ifndef INCLUDED_BUILDBOXCOMMON_REQUESTMETADATA
#define INCLUDED_BUILDBOXCOMMON_REQUESTMETADATA

#include <buildboxcommon_protos.h>
#include <string>

namespace buildboxcommon {

class RequestMetadataGenerator {

    /** According to the REAPI specification:
     *
     * "An optional Metadata to attach to any RPC request to tell the server
     about
     * an external context of the request."
     *
     * "[...] To use it, the client attaches the header to the call using the
     * canonical proto serialization[.]"

     * name: `build.bazel.remote.execution.v2.requestmetadata-bin`
     * contents: the base64 encoded binary `RequestMetadata` message.
     *
     *
     * This class helps to create that message and attach it to a
     * `grpc::ClientContext` object.
     */

  public:
    RequestMetadataGenerator();
    RequestMetadataGenerator(const std::string &tool_name,
                             const std::string &tool_version);

    void attach_request_metadata(grpc::ClientContext *context) const;

    void set_tool_details(const std::string &tool_name,
                          const std::string &tool_details);

    void set_action_id(const std::string &action_id);
    void set_tool_invocation_id(const std::string &tool_invocation_id);
    void set_correlated_invocations_id(
        const std::string &correlated_invocations_id);

  private:
    void attach_request_metadata(
        grpc::ClientContext *context, const std::string &action_id,
        const std::string &tool_invocation_id,
        const std::string &correlated_invocations_id) const;

  protected:
    ToolDetails d_tool_details;

    std::string d_action_id;
    std::string d_tool_invocation_id;
    std::string d_correlated_invocations_id;

    RequestMetadata generate_request_metadata(
        const std::string &action_id, const std::string &tool_invocation_id,
        const std::string &correlated_invocations_id) const;

    static const std::string HEADER_NAME;
};

} // namespace buildboxcommon

#endif // INCLUDED_BUILDBOXCOMMON_REQUESTMETADATA
