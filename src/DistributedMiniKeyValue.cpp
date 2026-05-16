#include "DistributedMiniKeyValue.hpp"

#include "StableStringHash.hpp"

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <stdexcept>

namespace {

std::vector<uint8_t> bytesFromString(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

} // namespace

DistributedMiniKeyValue::DistributedMiniKeyValue(std::vector<std::string> shard_endpoints) {
    if (shard_endpoints.empty()) {
        throw std::invalid_argument("DistributedMiniKeyValue requires at least one shard endpoint");
    }
    channels_.reserve(shard_endpoints.size());
    stubs_.reserve(shard_endpoints.size());
    for (const auto& ep : shard_endpoints) {
        auto ch = grpc::CreateChannel(ep, grpc::InsecureChannelCredentials());
        channels_.push_back(ch);
        stubs_.push_back(minikvalue::v1::KeyValueService::NewStub(ch));
    }
}

void DistributedMiniKeyValue::throwIfNotOk(const grpc::Status& st) {
    if (st.ok()) {
        return;
    }
    throw std::runtime_error(std::string("gRPC error: ") + std::to_string(static_cast<int>(st.error_code())) + " "
                             + st.error_message());
}

std::optional<std::vector<uint8_t>> DistributedMiniKeyValue::Get(const std::string& key) const {
    grpc::ClientContext ctx;
    minikvalue::v1::GetRequest req;
    req.set_key(key);
    minikvalue::v1::GetResponse resp;
    const std::size_t idx = shardIndexForKey(key, stubs_.size());
    const grpc::Status st = stubs_[idx]->Get(&ctx, req, &resp);
    throwIfNotOk(st);
    if (!resp.found()) {
        return std::nullopt;
    }
    return bytesFromString(resp.value());
}

void DistributedMiniKeyValue::Set(const std::string& key, const std::vector<uint8_t>& value) {
    grpc::ClientContext ctx;
    minikvalue::v1::SetRequest req;
    req.set_key(key);
    req.set_value(std::string(value.begin(), value.end()));
    minikvalue::v1::SetResponse resp;
    const std::size_t idx = shardIndexForKey(key, stubs_.size());
    throwIfNotOk(stubs_[idx]->Set(&ctx, req, &resp));
}

void DistributedMiniKeyValue::Delete(const std::string& key) {
    grpc::ClientContext ctx;
    minikvalue::v1::DeleteRequest req;
    req.set_key(key);
    minikvalue::v1::DeleteResponse resp;
    const std::size_t idx = shardIndexForKey(key, stubs_.size());
    throwIfNotOk(stubs_[idx]->Delete(&ctx, req, &resp));
}

std::unordered_map<std::string, std::vector<uint8_t>> DistributedMiniKeyValue::GetAllKeyValues() const {
    std::unordered_map<std::string, std::vector<uint8_t>> out;
    for (std::size_t i = 0; i < stubs_.size(); ++i) {
        grpc::ClientContext ctx;
        minikvalue::v1::ExportRequest req;
        std::unique_ptr<grpc::ClientReader<minikvalue::v1::KeyValuePair>> reader(stubs_[i]->Export(&ctx, req));
        minikvalue::v1::KeyValuePair kv;
        while (reader->Read(&kv)) {
            out[kv.key()] = bytesFromString(kv.value());
        }
        throwIfNotOk(reader->Finish());
    }
    return out;
}

void DistributedMiniKeyValue::BatchWrite(const std::vector<LogEntry>& entries) {
    std::vector<std::vector<LogEntry>> by_shard(stubs_.size());
    for (const auto& e : entries) {
        by_shard[shardIndexForKey(e.key, stubs_.size())].push_back(e);
    }
    for (std::size_t i = 0; i < stubs_.size(); ++i) {
        if (by_shard[i].empty()) {
            continue;
        }
        grpc::ClientContext ctx;
        minikvalue::v1::BatchWriteRequest req;
        for (const auto& e : by_shard[i]) {
            auto* m = req.add_entries();
            m->set_key(e.key);
            m->set_value(std::string(e.value.begin(), e.value.end()));
            m->set_is_delete(e.isDelete);
        }
        minikvalue::v1::BatchWriteResponse resp;
        throwIfNotOk(stubs_[i]->BatchWrite(&ctx, req, &resp));
    }
}
