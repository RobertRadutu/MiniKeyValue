#include "../include/MiniKeyValue.hpp"
#include "../include/FileStorage.hpp"
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace {

/// Serialized size of the live map if rewritten as non-delete log records (matches FileStorage format).
std::uint64_t estimateCompactedLogBytes(const std::unordered_map<std::string, std::vector<uint8_t>>& live) {
    std::uint64_t n = 0;
    for (const auto& kv : live) {
        n += static_cast<std::uint64_t>(sizeof(uint8_t) + sizeof(uint64_t) + kv.first.size()
                                        + sizeof(uint64_t) + kv.second.size());
    }
    return n;
}

} // namespace

MiniKeyValue::MiniKeyValue(std::unique_ptr<IStorage> storage, FactoryAccess)
    : storage_(std::move(storage)) {}

MiniKeyValue::~MiniKeyValue() = default;

std::shared_ptr<MiniKeyValue> MiniKeyValue::createEmpty(const std::string& path) {
    auto storage = std::make_unique<FileStorage>(path);
    storage->clear();
    return std::make_shared<MiniKeyValue>(std::move(storage), FactoryAccess{});
}

std::shared_ptr<MiniKeyValue> MiniKeyValue::createMiniKeyValue(const std::string& path) {
    auto storage = std::make_unique<FileStorage>(path);
    auto mkv = std::make_shared<MiniKeyValue>(std::move(storage), FactoryAccess{});
    mkv->readFile();
    return mkv;
}

std::shared_ptr<MiniKeyValue> MiniKeyValue::createFromStorage(std::unique_ptr<IStorage> storage) {
    storage->clear();
    return std::make_shared<MiniKeyValue>(std::move(storage), FactoryAccess{});
}

void MiniKeyValue::readFile() {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    store_.clear();
    storage_->replay([this](const LogEntry& entry) {
        if (entry.isDelete) {
            store_.erase(entry.key);
        } else {
            store_[entry.key] = entry.value;
        }
    });
}

std::optional<std::vector<uint8_t>> MiniKeyValue::Get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void MiniKeyValue::maybeCompactAfterMutation() {
    const std::uint64_t file_bytes = storage_->sizeBytes();
    const std::uint64_t minimal_bytes = estimateCompactedLogBytes(store_);

    if (minimal_bytes == 0) {
        if (file_bytes > 0) {
            storage_->compactFromSnapshot(store_);
        }
        return;
    }
    if (file_bytes > 2 * minimal_bytes) {
        storage_->compactFromSnapshot(store_);
    }
}

void MiniKeyValue::Set(const std::string& key, const std::vector<uint8_t>& value) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    store_[key] = value;
    storage_->write({key, value, false});
    maybeCompactAfterMutation();
}

void MiniKeyValue::Delete(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    store_.erase(key);
    storage_->write({key, {}, true});
    maybeCompactAfterMutation();
}

std::unordered_map<std::string, std::vector<uint8_t>> MiniKeyValue::GetAllKeyValues() const {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    return store_;
}
