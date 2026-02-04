#ifndef __FILE_STORAGE_HPP
#define __FILE_STORAGE_HPP

#include "IStorage.hpp"
#include <mutex>
#include <string>

class FileStorage : public IStorage {
public:
    explicit FileStorage(const std::string& path) : path_(path) {}
    void clear() override;
    void write(const LogEntry& entry) override;
    void replay(std::function<void(const LogEntry&)> callback) const override;
private:
    std::string path_;
    mutable std::mutex mutex_;
};

#endif //__FILE_STORAGE_HPP