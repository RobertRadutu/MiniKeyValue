#include "GrpcKeyValueServiceImpl.hpp"

#include "LogEntry.hpp"
#include "StableStringHash.hpp"

#include <string_view>

namespace {

std::vector<uint8_t> bytesFromString(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

} // namespace

GrpcKeyValueServiceImpl::GrpcKeyValueServiceImpl(std::shared_ptr<MiniKeyValue> store, int shard_index,
                                                 int shard_count)
    : store_(std::move(store)), shard_index_(shard_index), shard_count_(shard_count) {}

grpc::Status GrpcKeyValueServiceImpl::checkShardOwnsKey(const std::string& key) const {
    if (shard_count_ < 1) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "invalid shard_count");
    }
    if (shard_index_ < 0 || shard_index_ >= shard_count_) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "invalid shard_index");
    }
    const std::size_t owner =
        shardIndexForKey(key, static_cast<std::size_t>(shard_count_));
    if (static_cast<int>(owner) != shard_index_) {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "key not owned by this shard");
    }
    return grpc::Status::OK;
}

grpc::Status GrpcKeyValueServiceImpl::Get(grpc::ServerContext*, const minikvalue::v1::GetRequest* request,
                                          minikvalue::v1::GetResponse* response) {
    if (auto st = checkShardOwnsKey(request->key()); !st.ok()) {
        return st;
    }
    auto v = store_->Get(request->key());
    if (!v) {
        response->set_found(false);
        return grpc::Status::OK;
    }
    response->set_found(true);
    response->set_value(std::string(v->begin(), v->end()));
    return grpc::Status::OK;
}

grpc::Status GrpcKeyValueServiceImpl::Set(grpc::ServerContext*, const minikvalue::v1::SetRequest* request,
                                          minikvalue::v1::SetResponse*) {
    if (auto st = checkShardOwnsKey(request->key()); !st.ok()) {
        return st;
    }
    store_->Set(request->key(), bytesFromString(request->value()));
    return grpc::Status::OK;
}

grpc::Status GrpcKeyValueServiceImpl::Delete(grpc::ServerContext*, const minikvalue::v1::DeleteRequest* request,
                                            minikvalue::v1::DeleteResponse*) {
    if (auto st = checkShardOwnsKey(request->key()); !st.ok()) {
        return st;
    }
    store_->Delete(request->key());
    return grpc::Status::OK;
}

grpc::Status GrpcKeyValueServiceImpl::BatchWrite(grpc::ServerContext*,
                                                 const minikvalue::v1::BatchWriteRequest* request,
                                                 minikvalue::v1::BatchWriteResponse*) {
    for (const auto& e : request->entries()) {
        if (auto st = checkShardOwnsKey(e.key()); !st.ok()) {
            return st;
        }
    }
    std::vector<LogEntry> batch;
    batch.reserve(static_cast<std::size_t>(request->entries_size()));
    for (const auto& e : request->entries()) {
        LogEntry le;
        le.key = e.key();
        le.value = bytesFromString(e.value());
        le.isDelete = e.is_delete();
        batch.push_back(std::move(le));
    }
    store_->BatchWrite(batch);
    return grpc::Status::OK;
}

grpc::Status GrpcKeyValueServiceImpl::Export(grpc::ServerContext*, const minikvalue::v1::ExportRequest*,
                                             grpc::ServerWriter<minikvalue::v1::KeyValuePair>* writer) {
    const auto all = store_->GetAllKeyValues();
    for (const auto& kv : all) {
        if (!checkShardOwnsKey(kv.first).ok()) {
            continue;
        }
        minikvalue::v1::KeyValuePair msg;
        msg.set_key(kv.first);
        msg.set_value(std::string(kv.second.begin(), kv.second.end()));
        if (!writer->Write(msg)) {
            return grpc::Status(grpc::StatusCode::UNKNOWN, "client closed stream during export");
        }
    }
    return grpc::Status::OK;
}
