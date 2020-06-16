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

#ifndef INCLUDED_BUILDBOXCOMMON_LOGSTREAMWRITER
#define INCLUDED_BUILDBOXCOMMON_LOGSTREAMWRITER

#include <buildboxcommon_connectionoptions.h>

#include <google/bytestream/bytestream.grpc.pb.h>

#include <memory>
#include <string>

namespace buildboxcommon {

class LogStreamWriter final {
    /*
     * This class allows to perform `ByteStream.Write()` operations for a
     * LogStream service. Write requests are limited to appending at the
     * end of the previously-written contents.
     *
     * (A writer instance cannot not be shared across threads.)
     */
  public:
    LogStreamWriter(const std::string &resourceName,
                    const ConnectionOptions &connectionOptions);

    // Constructor method for unit testing (allows mocking the ByteStream
    // client.)
    LogStreamWriter(
        const std::string &resourceName,
        std::shared_ptr<google::bytestream::ByteStream::StubInterface>
            bytestreamClient,
        const int grpcRetryLimit, const int grpcRetryDelay);

    // Issue a `ByteStream.Write()` with the given data, appending it to the
    // previously written contents.
    // On errors throws a `GrpcError` exception.
    void write(const std::string &data);

    // Issue a last `ByteStream.Write()` with `set_finish_write == true` and
    // close the stream.
    // No further writes can be issued after calling this method and it must
    // be invoked only once.
    // On errors throws a `GrpcError` exception.
    void commit();

    // Copies are disallowed because ownership must be exclusive to the creator
    // of the instance.
    LogStreamWriter(const LogStreamWriter &) = delete;
    LogStreamWriter &operator=(LogStreamWriter const &) = delete;

  private:
    const std::string d_resourceName;
    const int d_grpcRetryLimit;
    const int d_grpcRetryDelay;

    std::shared_ptr<google::bytestream::ByteStream::StubInterface>
        d_byteStreamClient;

    // ByteStream writer object (needs to be maintained in scope to keep the
    // stream alive.)
    typedef std::unique_ptr<
        grpc::ClientWriterInterface<google::bytestream::WriteRequest>>
        ByteStreamClientWriter;

    // It will use this context:
    grpc::ClientContext d_clientContext;
    // And will write the `WriteResponse` message obtained on `commit()` to
    // this variable:
    google::bytestream::WriteResponse d_writeResponse;

    ByteStreamClientWriter d_bytestreamWriter;

    // Getter for the ByteStream writer object, which is lazily initialized on
    // the first call.
    ByteStreamClientWriter &bytestreamWriter();

    // Number of bytes that were successfully written so far:
    google::protobuf::int64 d_writeOffset;

    // Keeps track of whether `commit()` was called to error on subsequent
    // calls to `write()`.
    bool d_writeCommitted;
};
} // namespace buildboxcommon

#endif
