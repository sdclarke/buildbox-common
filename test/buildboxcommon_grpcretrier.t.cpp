#include <buildboxcommon_grpcretrier.h>
#include <buildboxcommon_requestmetadata.h>

#include <gtest/gtest.h>

#include <google/rpc/error_details.grpc.pb.h>

#include <chrono>
#include <functional>
#include <iostream>

using buildboxcommon::GrpcRetrier;
using buildboxcommon::GrpcRetrierFactory;

TEST(GrpcRetrier, TestDefaultRetriableCode)
{
    const int retryLimit = 4;
    const std::chrono::milliseconds retryDelay(150);

    auto lambda = [&](grpc::ClientContext &) { return grpc::Status::OK; };

    {
        GrpcRetrierFactory retrierFactory(retryLimit, retryDelay);
        GrpcRetrier r(retrierFactory.makeRetrier(lambda, "lambda()"));
        ASSERT_EQ(r.retryableStatusCodes().size(), 1);
        ASSERT_TRUE(
            r.retryableStatusCodes().count(grpc::StatusCode::UNAVAILABLE));
    }

    {
        GrpcRetrier r(retryLimit, retryDelay, lambda, "lambda()");
        ASSERT_EQ(r.retryableStatusCodes().size(), 1);
        ASSERT_TRUE(
            r.retryableStatusCodes().count(grpc::StatusCode::UNAVAILABLE));
    }
}

TEST(GrpcRetrier, TestGetters)
{
    const int retryLimit = 4;
    const std::chrono::milliseconds retryDelay(150);

    GrpcRetrierFactory retrierFactory(retryLimit, retryDelay);

    auto lambda = [&](grpc::ClientContext &) { return grpc::Status::OK; };

    GrpcRetrier r(retrierFactory.makeRetrier(lambda, "lambda()"));
    EXPECT_EQ(r.retryLimit(), retryLimit);
    EXPECT_EQ(r.retryDelayBase(), retryDelay);
}

TEST(GrpcRetrier, SimpleSucceedTest)
{
    const int retryLimit = 1;
    const std::chrono::milliseconds retryDelay(100);

    int numRequests = 0;
    auto lambda = [&](grpc::ClientContext &) {
        numRequests++;
        return grpc::Status::OK;
    };

    GrpcRetrier r(retryLimit, retryDelay, lambda, "lambda()");

    EXPECT_TRUE(r.issueRequest());
    EXPECT_EQ(numRequests, 1);
    EXPECT_TRUE(r.status().ok());
    EXPECT_EQ(r.retryAttempts(), 0);
}

TEST(GrpcRetrier, OtherException)
{
    const int retryLimit = 1;
    const std::chrono::milliseconds retryDelay(100);

    /* Suceed once, if called again fail */
    int failures = 0;
    auto lambda = [&](grpc::ClientContext &) {
        if (failures < 1) {
            failures++;
            return grpc::Status(grpc::DEADLINE_EXCEEDED, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    const GrpcRetrier::GrpcStatusCodes otherExceptions = {
        grpc::DEADLINE_EXCEEDED};
    GrpcRetrier r(retryLimit, retryDelay, lambda, "lambda()", otherExceptions);

    EXPECT_TRUE(r.issueRequest());
    EXPECT_TRUE(r.status().ok());
    EXPECT_EQ(r.retryAttempts(), 1);

    failures = -1;

    EXPECT_FALSE(r.issueRequest());
    EXPECT_EQ(r.status().error_code(), grpc::DEADLINE_EXCEEDED);
    EXPECT_EQ(r.status().error_message(), "failing in test");
    EXPECT_EQ(r.retryAttempts(), 1);
}

TEST(GrpcRetrier, MultipleException)
{
    const std::chrono::milliseconds retryDelay(100);
    const GrpcRetrier::GrpcStatusCodes otherExceptions = {
        grpc::DEADLINE_EXCEEDED, grpc::INVALID_ARGUMENT};

    unsigned int failures = 0;
    auto lambda = [&](grpc::ClientContext &) {
        switch (failures) {
            case 0:
                failures++;
                return grpc::Status(grpc::DEADLINE_EXCEEDED,
                                    "failing in test");
            case 1:
                failures++;
                return grpc::Status(grpc::INVALID_ARGUMENT, "failing in test");
            case 2:
                failures++;
                return grpc::Status(grpc::UNAVAILABLE, "failing in test");
            case 3:
                return grpc::Status::OK;
        }
        return grpc::Status::OK;
    };

    {
        unsigned int retryLimit = 3;
        GrpcRetrier r(retryLimit, retryDelay, lambda, "", otherExceptions);
        EXPECT_TRUE(r.issueRequest());
        EXPECT_TRUE(r.status().ok());
        EXPECT_EQ(r.retryAttempts(), 3);
    }

    failures = 0;

    {
        unsigned int retryLimit = 2;
        GrpcRetrier r(retryLimit, retryDelay, lambda, "", otherExceptions);
        EXPECT_FALSE(r.issueRequest());
        EXPECT_EQ(r.status().error_code(), grpc::UNAVAILABLE);
        EXPECT_EQ(r.status().error_message(), "failing in test");
        EXPECT_EQ(r.retryAttempts(), 2);
    }
}

TEST(GrpcRetrier, ExceptionNotIncluded)
{
    const int retryLimit = 3;
    const std::chrono::milliseconds retryDelay(100);
    const GrpcRetrier::GrpcStatusCodes otherExceptions = {
        grpc::DEADLINE_EXCEEDED, grpc::INVALID_ARGUMENT};

    int failures = 0;
    auto lambda = [&](grpc::ClientContext &) {
        switch (failures) {
            case 0: // Original attempt fails => retry
                failures++;
                return grpc::Status(grpc::DEADLINE_EXCEEDED,
                                    "failing in test");
            case 1: // Fail on retry #1 => retry again
                failures++;
                return grpc::Status(grpc::INVALID_ARGUMENT, "failing in test");
            case 2: // Fail on retry #2 w/ non-retryable error => abort
                failures++;
                return grpc::Status(grpc::PERMISSION_DENIED,
                                    "failing in test");
            case 3:
                return grpc::Status::OK;
        }
        return grpc::Status::OK;
    };

    GrpcRetrier r(retryLimit, retryDelay, lambda, "", otherExceptions);
    EXPECT_TRUE(r.issueRequest());
    EXPECT_EQ(r.status().error_code(), grpc::PERMISSION_DENIED);
    EXPECT_EQ(r.retryAttempts(), 2);
}

TEST(GrpcRetrier, SimpleRetrySucceedTest)
{
    const int retryLimit = 1;
    const std::chrono::milliseconds retryDelay(100);

    /* Fail once, then succeed. */
    int failures = 0;
    auto lambda = [&](grpc::ClientContext &) {
        if (failures < 1) {
            failures++;
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    GrpcRetrier r(retryLimit, retryDelay, lambda, "");
    EXPECT_TRUE(r.issueRequest());
    EXPECT_TRUE(r.status().ok());
    EXPECT_EQ(r.retryAttempts(), 1);
}

TEST(GrpcRetrier, SimpleRetryFailTest)
{
    const int retryLimit = 2;
    const std::chrono::milliseconds retryDelay(100);

    /* Fail three times, then succeed. */
    int failures = 0;
    auto lambda = [&](grpc::ClientContext &) {
        if (failures < 3) {
            failures++;
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    GrpcRetrier r(retryLimit, retryDelay, lambda, "");
    EXPECT_FALSE(r.issueRequest());
    EXPECT_EQ(r.status().error_code(), grpc::UNAVAILABLE);
    EXPECT_EQ(r.status().error_message(), "failing in test");
    EXPECT_EQ(r.retryAttempts(), 2);
}

TEST(GrpcRetrier, ServerProvidedDelay)
{
    const int retryLimit = 2;
    const std::chrono::milliseconds retryDelay(100);

    /* Fail one time, then succeed. */
    bool firstRequest = true;
    const std::chrono::milliseconds serverSpecifiedDelay(500);
    auto lambda = [&](grpc::ClientContext &) {
        if (!firstRequest) {
            return grpc::Status::OK;
        }

        firstRequest = false;

        google::protobuf::Duration delay;
        delay.set_seconds(0);
        delay.set_nanos(
            std::chrono::nanoseconds(serverSpecifiedDelay).count());

        google::rpc::RetryInfo retryInfo;
        *retryInfo.mutable_retry_delay() = delay;

        return grpc::Status(grpc::UNAVAILABLE, "failing in test",
                            retryInfo.SerializeAsString());
    };

    GrpcRetrier r(retryLimit, retryDelay, lambda, "");
    EXPECT_TRUE(r.issueRequest());
    EXPECT_TRUE(r.status().ok());
    EXPECT_EQ(r.retryAttempts(), 1);
    EXPECT_EQ(r.retryDelayBase(), serverSpecifiedDelay); // 500 ms
}

TEST(GrpcRetrier, AttachMetadata)
{
    buildboxcommon::RequestMetadataGenerator metadata_generator(
        "testing tool name", "v0.1");
    metadata_generator.set_action_id("action1");

    // Automatic success, no need to retry.
    auto grpc_invocation = [&](grpc::ClientContext &) {
        return grpc::Status::OK;
    };

    int attacher_calls = 0;
    auto metadata_attacher = [&](grpc::ClientContext *context) {
        metadata_generator.attach_request_metadata(context);
        attacher_calls++;
    };

    unsigned int retryLimit = 0;
    const std::chrono::milliseconds retryDelay(0);

    GrpcRetrierFactory retrierFactory(retryLimit, retryDelay,
                                      metadata_attacher);

    GrpcRetrier r(
        retrierFactory.makeRetrier(grpc_invocation, "grpc_invocation()"));
    r.setMetadataAttacher(metadata_attacher);

    ASSERT_TRUE(r.issueRequest());
    ASSERT_EQ(attacher_calls, 1);
}
