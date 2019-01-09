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

#include <iostream>

// Check that we can include buildbox-common headers
#include <buildbox-common.h>
#include <build/bazel/remote/execution/v2/remote_execution.grpc.pb.h>
#include <google/api/http.pb.h>

// Check that we can include a gRPC header
#include <grpcpp/grpcpp.h>

int main(int argc, char **argv)
{
	// Try using a protobuf
	build::bazel::remote::execution::v2::Digest digest;
	digest.set_hash("abcdef");
	if (digest.hash() != "abcdef") {
		std::cerr << "Failed to set digest hash!" << std::endl;
		return 1;
	}

	if (argc > 1) {
		// Try initializing a CAS client
		Client client;
		client.init(argv[1], nullptr, nullptr, nullptr);
	}

        std::cerr << "gRPC version: " << grpc::Version() << std::endl;
	std::cerr << "Tests passed!" << std::endl;
	return 0;
}
