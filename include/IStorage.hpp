#ifndef __STORAGE_HPP
#define __STORAGE_HPP

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

#include "LogEntry.hpp"

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual void clear() = 0;
    virtual void write(const LogEntry& e) = 0;
    virtual void replay(std::function<void(const LogEntry&)>) const = 0;
};

class FileStorage : public IStorage {
public:
    explicit FileStorage(const std::string& path) : path_(path) {}

    void clear() override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream ofs(path_, std::ios::binary | std::ios::trunc);
    }

    // void write(const LogEntry& entry) override {
    //     std::lock_guard<std::mutex> lock(mutex_);
    //     std::ofstream ofs{path_, std::ios::binary | std::ios::app};
    //     if(!ofs.is_open()){
    //         throw std::runtime_error("FileStorage: failed to open file for writing: " + path_);
    //     }

    //     uint8_t flag = entry.isDelete ? 1 : 0;
    //     ofs.write(reinterpret_cast<char*>(&flag), sizeof(flag));
    //     if(!ofs.good()){
    //         throw std::runtime_error("FileStorage: write failed for flag of entry with key: " + entry.key);
    //     }

    //     uint64_t ksize = entry.key.size();
    //     ofs.write(reinterpret_cast<const char*>(&ksize), sizeof(ksize));
    //     if(!ofs.good()){
    //         throw std::runtime_error("FileStorage: write failed for size of the key of entry with key: " + entry.key);
    //     }
    //     ofs.write(entry.key.data(), ksize);
    //     if(!ofs.good()){
    //         throw std::runtime_error("FileStorage: write failed for key of entry with key: " + entry.key);
    //     }

    //     if(!flag){
    //         uint64_t vsize = entry.value.size();
    //         ofs.write(reinterpret_cast<const char*>(&vsize), sizeof(vsize));
    //         if(!ofs.good()){
    //             throw std::runtime_error("FileStorage: write failed for size of value of entry with key: " + entry.key);
    //         }
    //         ofs.write(reinterpret_cast<const char*>(entry.value.data()), vsize);
    //         if(!ofs.good()){
    //             throw std::runtime_error("FileStorage: write failed for value of entry with key: " + entry.key);
    //         }
    //     }

    //     ofs.flush();
    //     if(!ofs.good()) {
    //         throw std::runtime_error("FileStorage: write failed for entry with key: " + entry.key);
    //     }
    // }

    void write(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<char> buffer;

        uint8_t flag = entry.isDelete == true ? 1 : 0;
        buffer.push_back(static_cast<char>(flag));

        uint64_t ksize = entry.key.size();
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&ksize), reinterpret_cast<const char*>(&ksize) + sizeof(ksize));
        buffer.insert(buffer.end(), entry.key.begin(), entry.key.end());

        if(!flag) {
            uint64_t vsize = entry.value.size();
            buffer.insert(buffer.end(), reinterpret_cast<const char*>(&vsize), reinterpret_cast<const char*>(&vsize) + sizeof(vsize)); 
            buffer.insert(buffer.end(), reinterpret_cast<const char*>(entry.value.data()), reinterpret_cast<const char*>(entry.value.data() + vsize));
        }

        std::ofstream ofs(path_, std::ios::binary | std::ios::app);
        if(!ofs.is_open()){
            throw std::runtime_error("FileStorage: failed to open file for writing: " + path_);
        }
        ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        try {
            ofs.write(buffer.data(), buffer.size());
            ofs.flush();
        }
        catch(const std::ios_base::failure& e) {
            throw std::runtime_error("FileStorage: write failed for entry with key: " + entry.key + " (" + e.what() + ").");
        }
    }

    void replay(std::function<void(const LogEntry&)> callback) const override {
        std::ifstream ifs(path_, std::ios::binary);
        if(!ifs.is_open()) {
            throw std::runtime_error("FileStorage: failed to open file for replay: " + path_);
        }
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            while(ifs.peek() != EOF) {
                LogEntry entry;
                
                uint8_t flag;
                ifs.read(reinterpret_cast<char*>(&flag), sizeof(flag));
                entry.isDelete = flag == 1 ? true : false;

                uint64_t ksize;
                ifs.read(reinterpret_cast<char*>(&ksize), sizeof(ksize));
                entry.key.resize(ksize);
                ifs.read(entry.key.data(), ksize);

                if(!entry.isDelete) {
                    uint64_t vsize;
                    ifs.read(reinterpret_cast<char*>(&vsize), sizeof(vsize));
                    entry.value.resize(vsize);
                    ifs.read(reinterpret_cast<char*>(entry.value.data()), vsize);
                }
                callback(entry);
            }
        } catch(const std::ios_base::failure& e) {
            throw std::runtime_error("FileStorage: replay failed!");
        }
    }

private:
    std::string path_;
    mutable std::mutex mutex_;
};

#endif // __STORAGE_HPP