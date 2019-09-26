// Copyright 2019 Bloomberg Finance L.P
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

#ifndef INCLUDED_BUILDBOXCOMMON_MERGEUTIL
#define INCLUDED_BUILDBOXCOMMON_MERGEUTIL

#include <buildboxcommon_merklize.h>
#include <buildboxcommon_protos.h>

#include <google/protobuf/repeated_field.h>

#include <sstream>
#include <vector>

namespace buildboxcommon {

/**
 * Example usage:
 *    buildboxcommon::Client client;
 *
 *    // assuming we've been give both digests
 *    // get the tree's for both the input and template digests;
 *    MergeUtil::DirectoryTree inputTree, templateTree;
 *    try {
 *        inputTree = client.getTree(inputDigest);
 *        templateTree = client.getTree(templateDigest);
 *    }
 *    catch (const std::runtime_error &e) {
 *        // handle exception...
 *    }
 *
 *    // merge the two trees
 *    Digest mergedRootDigest;
 *    digest_string_map dsMap;
 *    const bool result = MergeUtil::createMergedDigest(
 *        inputTree, templateTree,
 *        &mergedRootDigest, &dsMap);
 *
 *    // save the newly generated digests to CAS
 *    grpc::ClientContext context;
 *    BatchUpdateBlobsRequest updateRequest;
 *    BatchUpdateBlobsResponse updateResponse;
 *    for (const auto &it : dsMap) {
 *        auto *request = updateRequest.add_requests();
 *        request->mutable_digest()->CopyFrom(it.first);
 *        request->mutable_data()->assign(it.second);
 *    }
 *    casClient->BatchUpdateBlobs(&context, updateRequest, &updateResponse);
 */

struct MergeUtil {
    typedef std::vector<Directory> DirectoryTree;

    /**
     * Create a merged Directory tree made up of the sum of all the
     * parts of the 2 input Directory's. Save the merged root digest
     * to 'rootDigest'. Optionally, save all the digests created
     * into 'dsmap'
     *
     * Returns true on success, false if collisions were detected
     */
    static bool createMergedDigest(const DirectoryTree &inputTree,
                                   const DirectoryTree &templateTree,
                                   Digest *rootDigest,
                                   digest_string_map *dsMap = nullptr);
};

// convenience streaming operators
std::ostream &operator<<(std::ostream &out,
                         const MergeUtil::DirectoryTree &tree);

std::ostream &operator<<(
    std::ostream &out,
    const ::google::protobuf::RepeatedPtrField<buildboxcommon::Directory>
        &tree);

} // namespace buildboxcommon

#endif
