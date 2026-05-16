#include <gtest/gtest.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/security/server_credentials.h>

#include "DistributedMiniKeyValue.hpp"
#include "GrpcKeyValueServiceImpl.hpp"
#include "MiniKeyValue.hpp"
#include "StableStringHash.hpp"
#include "keyvalue.grpc.pb.h"

#include <memory>
#include <string>

namespace {

std::pair<std::string, std::string> findKeysOnDifferentShards(std::size_t shard_count) {
    std::string k0;
    std::string k1;
    for (int i = 0; i < 100000; ++i) {
        const std::string k = "probe" + std::to_string(i);
        if (shardIndexForKey(k, shard_count) == 0 && k0.empty()) {
            k0 = k;
        }
        if (shardIndexForKey(k, shard_count) == 1 && k1.empty()) {
            k1 = k;
        }
        if (!k0.empty() && !k1.empty()) {
            break;
        }
    }
    return {k0, k1};
}

} // namespace

TEST(GrpcSharded, singleShardRoundTrip) {
    const std::string data_path = std::string(::testing::TempDir()) + "single_shard.mkv";

    auto mkv = MiniKeyValue::createEmpty(data_path);
    auto impl = std::make_unique<GrpcKeyValueServiceImpl>(mkv, 0, 1);

    int port = 0;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(impl.get());
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    ASSERT_NE(server, nullptr);
    ASSERT_GT(port, 0);

    DistributedMiniKeyValue dist({"127.0.0.1:" + std::to_string(port)});
    dist.Set("hello", {9, 8, 7});
    auto got = dist.Get("hello");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(*got, (std::vector<uint8_t>{9, 8, 7}));
    dist.Delete("hello");
    EXPECT_FALSE(dist.Get("hello").has_value());

    std::vector<LogEntry> batch;
    batch.push_back({"a", {1}, false});
    batch.push_back({"b", {2}, false});
    batch.push_back({"a", {}, true});
    dist.BatchWrite(batch);
    EXPECT_FALSE(dist.Get("a").has_value());
    ASSERT_TRUE(dist.Get("b").has_value());

    server->Shutdown();
}

TEST(GrpcSharded, twoShardsRoundTripAndExport) {
    const std::string base = ::testing::TempDir();
    const std::string p0 = base + "shard0.mkv";
    const std::string p1 = base + "shard1.mkv";

    auto mkv0 = MiniKeyValue::createEmpty(p0);
    auto mkv1 = MiniKeyValue::createEmpty(p1);
    auto impl0 = std::make_unique<GrpcKeyValueServiceImpl>(mkv0, 0, 2);
    auto impl1 = std::make_unique<GrpcKeyValueServiceImpl>(mkv1, 1, 2);

    int port0 = 0;
    int port1 = 0;
    grpc::ServerBuilder b0;
    b0.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port0);
    b0.RegisterService(impl0.get());
    std::unique_ptr<grpc::Server> s0 = b0.BuildAndStart();
    ASSERT_NE(s0, nullptr);

    grpc::ServerBuilder b1;
    b1.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port1);
    b1.RegisterService(impl1.get());
    std::unique_ptr<grpc::Server> s1 = b1.BuildAndStart();
    ASSERT_NE(s1, nullptr);

    const auto [k0, k1] = findKeysOnDifferentShards(2);
    ASSERT_FALSE(k0.empty());
    ASSERT_FALSE(k1.empty());

    DistributedMiniKeyValue dist(
        {"127.0.0.1:" + std::to_string(port0), "127.0.0.1:" + std::to_string(port1)});

    dist.Set(k0, {1, 2});
    dist.Set(k1, {3, 4, 5});
    ASSERT_TRUE(dist.Get(k0).has_value());
    ASSERT_TRUE(dist.Get(k1).has_value());
    EXPECT_EQ(*dist.Get(k0), (std::vector<uint8_t>{1, 2}));
    EXPECT_EQ(*dist.Get(k1), (std::vector<uint8_t>{3, 4, 5}));

    const auto merged = dist.GetAllKeyValues();
    ASSERT_EQ(merged.size(), 2u);
    EXPECT_EQ(merged.at(k0), (std::vector<uint8_t>{1, 2}));
    EXPECT_EQ(merged.at(k1), (std::vector<uint8_t>{3, 4, 5}));

    s0->Shutdown();
    s1->Shutdown();
}

TEST(GrpcSharded, wrongShardRejectedByServer) {
    const std::string base = ::testing::TempDir();
    const std::string p0 = base + "wrong_shard.mkv";

    auto mkv = MiniKeyValue::createEmpty(p0);
    auto impl = std::make_unique<GrpcKeyValueServiceImpl>(mkv, 0, 2);

    int port = 0;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(impl.get());
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    ASSERT_NE(server, nullptr);

    const auto [k0, k1] = findKeysOnDifferentShards(2);
    ASSERT_FALSE(k0.empty());
    ASSERT_FALSE(k1.empty());

    auto channel = grpc::CreateChannel("127.0.0.1:" + std::to_string(port), grpc::InsecureChannelCredentials());
    auto stub = minikvalue::v1::KeyValueService::NewStub(channel);

    {
        grpc::ClientContext ctx;
        minikvalue::v1::SetRequest req;
        req.set_key(k0);
        req.set_value("ok");
        minikvalue::v1::SetResponse resp;
        EXPECT_TRUE(stub->Set(&ctx, req, &resp).ok());
    }
    {
        grpc::ClientContext ctx;
        minikvalue::v1::SetRequest req;
        req.set_key(k1);
        req.set_value("bad");
        minikvalue::v1::SetResponse resp;
        const grpc::Status st = stub->Set(&ctx, req, &resp);
        EXPECT_EQ(st.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
    }

    server->Shutdown();
}
