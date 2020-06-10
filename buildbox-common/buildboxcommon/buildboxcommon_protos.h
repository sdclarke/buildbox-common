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

#ifndef INCLUDED_BUILDBOXCOMMON_PROTOS
#define INCLUDED_BUILDBOXCOMMON_PROTOS

#include <build/bazel/remote/asset/v1/remote_asset.grpc.pb.h>
#include <build/bazel/remote/execution/v2/remote_execution.grpc.pb.h>
#include <build/buildgrid/local_cas.grpc.pb.h>
#include <google/bytestream/bytestream.grpc.pb.h>

namespace buildboxcommon {

using namespace google::bytestream;
using namespace build::bazel::remote::asset::v1;
using namespace build::bazel::remote::execution::v2;
using namespace build::buildgrid;

} // namespace buildboxcommon

// Allow Digests to be used as unordered_hash keys.
namespace std {
template <> struct hash<buildboxcommon::Digest> {
    std::size_t operator()(const buildboxcommon::Digest &digest) const noexcept
    {
        return std::hash<std::string>{}(digest.hash());
    }
};
} // namespace std

namespace build {
namespace bazel {
namespace remote {
namespace execution {
namespace v2 {
inline bool operator==(const buildboxcommon::Digest &a,
                       const buildboxcommon::Digest &b)
{
    return a.hash() == b.hash() && a.size_bytes() == b.size_bytes();
}

inline bool operator!=(const buildboxcommon::Digest &a,
                       const buildboxcommon::Digest &b)
{
    return !(a == b);
}

inline std::string toString(const buildboxcommon::Digest &digest)
{
    return digest.hash() + "/" + std::to_string(digest.size_bytes());
}

inline std::ostream &operator<<(std::ostream &os,
                                const buildboxcommon::Digest &digest)
{
    return os << toString(digest);
}

} // namespace v2
} // namespace execution
} // namespace remote
} // namespace bazel
} // namespace build

#endif
