#ifndef __MINIKEYVALUE_HPP
#define __MINIKEYVALUE_HPP

#include <unordered_map>
#include <thread>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "IStorage.hpp"

class MiniKeyValue{
public:
    explicit MiniKeyValue(std::unique_ptr<IStorage> storage) : storage_(std::move(storage)) {}
    static std::shared_ptr<MiniKeyValue> createEmpty(const std::string& path) {
        auto storage = std::make_unique<FileStorage>(path);
        storage->clear();
        return std::make_shared<MiniKeyValue>(std::move(storage));
    }

    static std::shared_ptr<MiniKeyValue> createMiniKeyValue(const std::string& path) {
        auto storage = std::make_unique<FileStorage>(path);
        auto mkv =  std::make_shared<MiniKeyValue>(std::move(storage));
        mkv->readFile();
        return mkv;
    }

    std::vector<uint8_t> Get(const std::string&) const;
    void Set(const std::string&, const std::vector<uint8_t>&);
    void Delete(const std::string&);
    void readFile(){
        std::lock_guard<std::mutex> lock(mtx);
        store.clear();
        storage_->replay([this](const LogEntry& entry){
            if(!entry.isDelete) {
                store[entry.key] = entry.value;
            }
            else {
                store.erase(entry.key);
            }
        });
    }

    std::unordered_map<std::string, std::vector<uint8_t>> GetAllKeyValues() const {
        std::lock_guard<std::mutex> lock(mtx);
        return store;
    }
    
private:
    bool isKey(const std::string&) const;
    std::vector<std::string> getAllKeys() const;
    void resetFile();

    std::unique_ptr<IStorage> storage_;
    std::unordered_map<std::string, std::vector<uint8_t>> store;
    mutable std::mutex mtx;
    std::string logFile;
};

#endif //__MINIKEYVALUE_HPP

/**
 * [Storage] -> bytes -> LogEntry
 * [Engine] -> LogEntry -> state
 */