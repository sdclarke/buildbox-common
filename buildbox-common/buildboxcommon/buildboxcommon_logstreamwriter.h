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
#include <buildboxcommon_grpcretrier.h>

#include <buildboxcommon_protos.h>

#include <chrono>
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
     *
     * When `write()` is called for the first time, it issues a
     * `QueryWriteStatus()` request to the server. This allows implementations
     * like BuildGrid to open the stream on-demand when at least a client is
     * reading the corresponding endpoint, thus avoiding unnecessary transfers
     * of data.
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
    // previously written contents and returns whether the write succeeded.
    //
    // The first call to this method will issue a `QueryWriteStatus()` request
    // to verify that the stream is available. Note that some implementations,
    // like BuildGrid, might block on that call for a long time. If the stream
    // is not available for writing, returns `false`.
    bool write(const std::string &data);

    // Issue a last `ByteStream.Write()` with `set_finish_write == true` and
    // close the stream.
    // No further writes can be issued after calling this method and it must
    // be invoked only once.
    // Returns whether the commit operation succeeded.
    bool commit();

    // Copies are disallowed because ownership must be exclusive to the creator
    // of the instance.
    LogStreamWriter(const LogStreamWriter &) = delete;
    LogStreamWriter &operator=(LogStreamWriter const &) = delete;

    // Issue a `CreateLogStream()` call to the given remote. Set the value of
    // `parent` in the request.
    static LogStream createLogStream(
        const std::string &parent,
        const buildboxcommon::ConnectionOptions &connectionOptions);

    static LogStream
    createLogStream(const std::string &parent, const int retryLimit,
                    const int retryDelay,
                    LogStreamService::StubInterface *logstreamClient);

  private:
    const std::string d_resourceName;
    const unsigned int d_grpcRetryLimit;
    const std::chrono::milliseconds d_grpcRetryDelay;

    std::shared_ptr<google::bytestream::ByteStream::StubInterface>
        d_byteStreamClient;

    // ByteStream writer object (needs to be maintained in scope to keep the
    // stream alive.)
    typedef std::unique_ptr<
        grpc::ClientWriterInterface<google::bytestream::WriteRequest>>
        ByteStreamClientWriter;

    GrpcRetrierFactory d_grpcRetrierFactory;

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

    // Keeps track of whether the resource is ready for writes. That is, we
    // successfully received a `QueryWriteStatus()` response.
    // Before that, no writes will be issued.
    bool d_resourceReady;

    // Issue a `QueryWriteStatus()` call for `d_resourceName` and return
    // whether the server returned an OK status.
    bool queryStreamWriteStatus() const;
};
} // namespace buildboxcommon

#endif
