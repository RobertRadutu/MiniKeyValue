#pragma once

#include <grpcpp/grpcpp.h>

#include <memory>

#include "MiniKeyValue.hpp"
#include "keyvalue.grpc.pb.h"

/// gRPC service for one shard: wraps MiniKeyValue and enforces key → shard routing.
class GrpcKeyValueServiceImpl final : public minikvalue::v1::KeyValueService::Service {
public:
    GrpcKeyValueServiceImpl(std::shared_ptr<MiniKeyValue> store, int shard_index, int shard_count);

    grpc::Status Get(grpc::ServerContext* context, const minikvalue::v1::GetRequest* request,
                     minikvalue::v1::GetResponse* response) override;

    grpc::Status Set(grpc::ServerContext* context, const minikvalue::v1::SetRequest* request,
                     minikvalue::v1::SetResponse* response) override;

    grpc::Status Delete(grpc::ServerContext* context, const minikvalue::v1::DeleteRequest* request,
                        minikvalue::v1::DeleteResponse* response) override;

    grpc::Status BatchWrite(grpc::ServerContext* context, const minikvalue::v1::BatchWriteRequest* request,
                            minikvalue::v1::BatchWriteResponse* response) override;

    grpc::Status Export(grpc::ServerContext* context, const minikvalue::v1::ExportRequest* request,
                        grpc::ServerWriter<minikvalue::v1::KeyValuePair>* writer) override;

private:
    grpc::Status checkShardOwnsKey(const std::string& key) const;

    std::shared_ptr<MiniKeyValue> store_;
    int shard_index_{0};
    int shard_count_{1};
};
