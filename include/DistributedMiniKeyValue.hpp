#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <grpcpp/channel.h>

#include "LogEntry.hpp"
#include "keyvalue.grpc.pb.h"

/// Client for a sharded deployment: constructor takes one gRPC target per shard,
/// ordered by shard index (0 .. N-1). Keys are routed with the same FNV-1a rule as
/// GrpcKeyValueServiceImpl (see StableStringHash.hpp).
class DistributedMiniKeyValue {
public:
    /// @param shard_endpoints i-th string is the target for shard i (e.g. "127.0.0.1:50051").
    explicit DistributedMiniKeyValue(std::vector<std::string> shard_endpoints);

    std::size_t shardCount() const { return stubs_.size(); }

    std::optional<std::vector<uint8_t>> Get(const std::string& key) const;
    void Set(const std::string& key, const std::vector<uint8_t>& value);
    void Delete(const std::string& key);
    std::unordered_map<std::string, std::vector<uint8_t>> GetAllKeyValues() const;
    void BatchWrite(const std::vector<LogEntry>& entries);

private:
    std::vector<std::shared_ptr<grpc::Channel>> channels_;
    std::vector<std::unique_ptr<minikvalue::v1::KeyValueService::Stub>> stubs_;

    static void throwIfNotOk(const grpc::Status& st);
};
