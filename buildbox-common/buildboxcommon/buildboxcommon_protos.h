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
#include <build/bazel/remote/logstream/v1/remote_logstream.grpc.pb.h>
#include <build/buildgrid/local_cas.grpc.pb.h>
#include <google/bytestream/bytestream.grpc.pb.h>

#include <buildboxcommon_exception.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace buildboxcommon {

using namespace google::bytestream;
using namespace build::bazel::remote::asset::v1;
using namespace build::bazel::remote::execution::v2;
using namespace build::bazel::remote::logstream::v1;
using namespace build::buildgrid;

struct ProtoUtils {
    template <typename T>
    static void writeProtobufToFile(const T &proto, const std::string &path)
    {
        const int fd = open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666);
        if (fd == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Could not open [" << path << "]");
        }

        const bool writeSucceeded = proto.SerializeToFileDescriptor(fd);
        close(fd);

        if (!writeSucceeded) {
            BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                           "Failed to write protobuf to ["
                                               << path << "]");
        }
    }

    template <typename T>
    static T readProtobufFromFile(const std::string &path)
    {
        const int fd = open(path.c_str(), O_RDONLY);
        if (fd == -1) {
            BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
                std::system_error, errno, std::system_category,
                "Could not open [" << path << "]");
        }

        T proto;
        const bool readSucceeded = proto.ParseFromFileDescriptor(fd);
        close(fd);

        if (readSucceeded) {
            return proto;
        }

        BUILDBOXCOMMON_THROW_EXCEPTION(std::runtime_error,
                                       "Failed to parse protobuf from ["
                                           << path << "]");
    }
};

} // namespace buildboxcommon

// Allow Digests to be used as unordered_hash keys.
namespace std {
template <> struct hash<buildboxcommon::Digest> {
    std::size_t operator()(const buildboxcommon::Digest &digest) const noexcept
    {
        return std::hash<std::string>{}(digest.hash_other());
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
    return a.hash_other() == b.hash_other() && a.size_bytes() == b.size_bytes();
}

inline bool operator!=(const buildboxcommon::Digest &a,
                       const buildboxcommon::Digest &b)
{
    return !(a == b);
}

inline bool operator<(const buildboxcommon::Digest &a,
                      const buildboxcommon::Digest &b)
{
    if (a.hash_other() != b.hash_other()) {
        return a.hash_other() < b.hash_other();
    }

    return a.size_bytes() < b.size_bytes();
}

inline std::string toString(const buildboxcommon::Digest &digest)
{
    return digest.hash_other() + "/" + std::to_string(digest.size_bytes());
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
