#ifndef MINIKVALUE_FILE_STORAGE_HPP
#define MINIKVALUE_FILE_STORAGE_HPP

#include "IStorage.hpp"
#include <LogEntry.hpp>
#include <mutex>
#include <string>

class FileStorage : public IStorage {
public:
    explicit FileStorage(const std::string& path) : path_(path) {}

    void clear() override;
    void write(const LogEntry& entry) override;
    void write(const std::vector<LogEntry>& entries) override;
    void replay(std::function<void(const LogEntry&)> callback) const override;

    std::uint64_t sizeBytes() const override;
    void compactFromSnapshot(const std::unordered_map<std::string, std::vector<uint8_t>>& live) override;

private:
    std::string path_;
    mutable std::mutex mutex_;
};

#endif // MINIKVALUE_FILE_STORAGE_HPP
