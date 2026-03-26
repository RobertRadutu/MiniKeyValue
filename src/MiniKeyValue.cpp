#include "../include/MiniKeyValue.hpp"
#include "../include/FileStorage.hpp"
#include <memory>
#include <mutex>

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

void MiniKeyValue::readFile() {
    std::lock_guard<std::mutex> lock(mtx_);
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
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void MiniKeyValue::Set(const std::string& key, const std::vector<uint8_t>& value) {
    std::lock_guard<std::mutex> lock(mtx_);
    store_[key] = value;
    storage_->write({key, value, false});
}

void MiniKeyValue::Delete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx_);
    store_.erase(key);
    storage_->write({key, {}, true});
}

std::unordered_map<std::string, std::vector<uint8_t>> MiniKeyValue::GetAllKeyValues() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return store_;
}
