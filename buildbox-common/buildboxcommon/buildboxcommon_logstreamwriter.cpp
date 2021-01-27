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
#include <buildboxcommon_logstreamwriter.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_logging.h>

#include <sstream>

namespace buildboxcommon {

LogStreamWriter::LogStreamWriter(const std::string &resourceName,
                                 const ConnectionOptions &connectionOptions)
    : LogStreamWriter(resourceName,
                      ByteStream::NewStub(connectionOptions.createChannel()),
                      std::stoi(connectionOptions.d_retryLimit),
                      std::stoi(connectionOptions.d_retryDelay))
{
}

LogStreamWriter::LogStreamWriter(
    const std::string &resourceName,
    std::shared_ptr<ByteStream::StubInterface> bytestreamClient,
    const int grpcRetryLimit, const int grpcRetryDelay)
    : d_resourceName(resourceName),
      d_grpcRetryLimit(static_cast<unsigned int>(grpcRetryLimit)),
      d_grpcRetryDelay(grpcRetryDelay), d_byteStreamClient(bytestreamClient),
      d_grpcRetrierFactory(d_grpcRetryLimit, d_grpcRetryDelay),
      d_writeOffset(0), d_writeCommitted(false), d_resourceReady(false)
{
}

bool LogStreamWriter::write(const std::string &data)
{
    if (d_writeCommitted) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "Attempted to `write()` after `commit()`.")
    }

    if (!d_resourceReady) {
        BUILDBOX_LOG_DEBUG("First call to `write()`. Issuing a "
                           "`QueryWriteStatus()` request and waiting "
                           "for it to return...");

        d_resourceReady = queryStreamWriteStatus();
        // Implementations like BuildGrid might block this call on the server
        // side until a reader activates the stream so that we don't send data
        // that nobody reads.
        // `QueryWriteStatus()` might also return `NOT_FOUND`, which indicates
        // that no readers were interested and therefore we don't need to send
        // any data.

        if (d_resourceReady) {
            BUILDBOX_LOG_DEBUG(
                "`QueryWriteStatus()` returned sucessfully. We can "
                "now start writing to the stream.");
        }
        else {
            BUILDBOX_LOG_DEBUG("`QueryWriteStatus()` failed. Aborting the "
                               "call to `ByteStream.Write()`");
            return false;
        }
    }

    WriteRequest request;
    request.set_resource_name(d_resourceName);
    request.set_write_offset(d_writeOffset);
    request.set_data(data);
    request.set_finish_write(false);

    auto writeLambda = [&request, this](grpc::ClientContext &) {
        if (!bytestreamWriter()->Write(request)) {
            const auto errorMessage = "Upload failed: broken stream";
            BUILDBOX_LOG_DEBUG(errorMessage);
            return grpc::Status(grpc::StatusCode::INTERNAL, errorMessage);
        }

        return grpc::Status::OK;
    };

    GrpcRetrier retrier(d_grpcRetryLimit, d_grpcRetryDelay, writeLambda,
                        "ByteStream.Write()");

    if (retrier.issueRequest() && retrier.status().ok()) {
        d_writeOffset += data.size();
        return true;
    }
    return false;
}

bool LogStreamWriter::commit()
{
    if (d_writeCommitted) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Attempted to `commit()` an already commited write.")
    }

    WriteRequest request;
    request.set_resource_name(d_resourceName);
    request.set_write_offset(d_writeOffset);
    request.set_finish_write(true);

    auto commitWriteLambda = [&request, this](grpc::ClientContext &) {
        if (!bytestreamWriter()->Write(request)) {
            const auto errorMessage = "Upload failed: broken stream";
            BUILDBOX_LOG_DEBUG(errorMessage);
            return grpc::Status(grpc::StatusCode::INTERNAL, errorMessage);
        }

        bytestreamWriter()->WritesDone();

        const auto status = bytestreamWriter()->Finish();
        if (status.ok()) {
            const auto bytesWritten = d_writeOffset;
            if (d_writeResponse.committed_size() != bytesWritten) {
                std::stringstream errorMessage;
                errorMessage << "Server reported uncommitted data: "
                             << d_writeResponse.committed_size() << " of "
                             << bytesWritten << " bytes";
                BUILDBOX_LOG_DEBUG(errorMessage.str());
                return grpc::Status(grpc::StatusCode::DATA_LOSS,
                                    errorMessage.str());
            }
        }

        return status;
    };

    GrpcRetrier retrier(d_grpcRetryLimit, d_grpcRetryDelay, commitWriteLambda,
                        "ByteStream.Write(set_finish_write=True)");

    if (retrier.issueRequest() && retrier.status().ok()) {
        d_writeCommitted = true;
        return true;
    }

    return false;
}

bool LogStreamWriter::queryStreamWriteStatus() const
{
    grpc::ClientContext context;

    QueryWriteStatusRequest request;
    request.set_resource_name(d_resourceName);

    auto queryWriteStatusLambda = [&context, &request,
                                   this](grpc::ClientContext &) {
        QueryWriteStatusResponse response;

        return d_byteStreamClient->QueryWriteStatus(&context, request,
                                                    &response);
    };

    GrpcRetrier retrier(d_grpcRetryLimit,
                        std::chrono::milliseconds(d_grpcRetryDelay),
                        queryWriteStatusLambda, "QueryWriteStatus()");

    return retrier.issueRequest() && retrier.status().ok();
}

LogStreamWriter::ByteStreamClientWriter &LogStreamWriter::bytestreamWriter()
{
    if (d_bytestreamWriter == nullptr) {
        d_bytestreamWriter =
            d_byteStreamClient->Write(&d_clientContext, &d_writeResponse);
    }

    return d_bytestreamWriter;
}

LogStream LogStreamWriter::createLogStream(
    const std::string &parent,
    const buildboxcommon::ConnectionOptions &connectionOptions)
{
    const auto channel = connectionOptions.createChannel();
    std::unique_ptr<LogStreamService::StubInterface> logStreamClient =
        LogStreamService::NewStub(channel);

    const int retryLimit = std::stoi(connectionOptions.d_retryLimit);
    const int retryDelay = std::stoi(connectionOptions.d_retryDelay);

    return createLogStream(parent, retryLimit, retryDelay,
                           logStreamClient.get());
}

LogStream LogStreamWriter::createLogStream(
    const std::string &parent, const int retryLimit, const int retryDelay,
    LogStreamService::StubInterface *logstreamClient)
{
    if (logstreamClient == nullptr) {
        BUILDBOXCOMMON_THROW_EXCEPTION(std::invalid_argument,
                                       "logstreamClient argument is nullptr");
    }

    grpc::ClientContext context;

    CreateLogStreamRequest request;
    request.set_parent(parent);

    LogStream createdLogStream;
    auto createLogStreamLambda = [&context, &request, &logstreamClient,
                                  &createdLogStream](grpc::ClientContext &) {
        return logstreamClient->CreateLogStream(&context, request,
                                                &createdLogStream);
    };

    GrpcRetrier retrier(retryLimit, std::chrono::milliseconds(retryDelay),
                        createLogStreamLambda, "CreateLogStream()");

    if (retrier.issueRequest() && retrier.status().ok()) {
        return createdLogStream;
    }

    const auto errorMessage = "CreateLogStream() failed: [" +
                              std::to_string(retrier.status().error_code()) +
                              ": " + retrier.status().error_message() + "]";
    throw GrpcError("CreateLogStream() failed", retrier.status());
}

} // namespace buildboxcommon
