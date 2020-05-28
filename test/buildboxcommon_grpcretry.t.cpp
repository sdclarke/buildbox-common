#include <buildboxcommon_grpcretry.h>
#include <buildboxcommon_requestmetadata.h>

#include <gtest/gtest.h>

#include <functional>
#include <iostream>

using buildboxcommon::GrpcRetry;

TEST(GrpcRetry, SimpleSucceedTest)
{
    int failures = 0;
    int retryLimit = 1;
    int retryDelay = 100;

    /* Suceed once, if called again fail */
    auto lambda = [&](grpc::ClientContext &context) {
        if (failures < 1) {
            failures++;
            return grpc::Status::OK;
        }
        else {
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
    };

    EXPECT_NO_THROW(GrpcRetry::retry(lambda, retryLimit, retryDelay));
}

TEST(GrpcRetry, SimpleRetrySucceedTest)
{
    int failures = 0;
    int retryLimit = 1;
    int retryDelay = 100;

    /* Fail once, then succeed. */
    auto lambda = [&](grpc::ClientContext &context) {
        if (failures < 1) {
            failures++;
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    EXPECT_NO_THROW(GrpcRetry::retry(lambda, retryLimit, retryDelay));
}

TEST(GrpcRetry, SimpleRetryFailTest)
{
    int failures = 0;
    int retryLimit = 2;
    int retryDelay = 100;

    /* Fail three times, then succeed. */
    auto lambda = [&](grpc::ClientContext &context) {
        if (failures < 3) {
            failures++;
            return grpc::Status(grpc::UNAVAILABLE, "failing in test");
        }
        else {
            return grpc::Status::OK;
        }
    };

    EXPECT_THROW(GrpcRetry::retry(lambda, retryLimit, retryDelay),
                 std::runtime_error);
}

TEST(GrpcRetry, AttachMetadata)
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

    EXPECT_NO_THROW(
        GrpcRetry::retry(grpc_invocation, 0, 0, metadata_attacher));
    ASSERT_EQ(attacher_calls, 1);
}
