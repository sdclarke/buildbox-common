/*
 * Copyright 2018 Bloomberg Finance LP
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

#include <buildboxcommon_client.h>
#include <buildboxcommon_connectionoptions.h>

#include <build/bazel/remote/execution/v2/remote_execution.grpc.pb.h>
#include <google/api/http.pb.h>
#include <grpcpp/grpcpp.h>
#include <iostream>

using namespace buildboxcommon;

static const int SAMPLE_USAGE_PAD_WIDTH = 30;

int main(int argc, char **argv)
{
    // Try using a protobuf
    Digest digest;
    digest.set_hash("abcdef");
    if (digest.hash() != "abcdef") {
        std::cerr << "Failed to set digest hash!" << std::endl;
        return 1;
    }

    if (argc > 1) {
        // Try initializing a CAS client
        ConnectionOptions opts;
        for (int i = 1; i < argc; ++i) {
            if (!opts.parseArg(argv[i])) {
                std::cerr << "Ignoring unexpected argument: " << argv[1]
                          << std::endl;
            }
        }
        Client client;
        client.init(opts);
    }

    std::cerr << "gRPC version: " << grpc::Version() << std::endl;
    std::cerr << "ConnectionOptions argument help:" << std::endl;
    ConnectionOptions::printArgHelp(SAMPLE_USAGE_PAD_WIDTH);
    std::cerr << "Tests passed!" << std::endl;
    return 0;
}
