#include "../include/FileStorage.hpp"
#include <fstream>
#include <stdexcept>

void FileStorage::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream ofs(path_, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        throw std::runtime_error("FileStorage: failed to open file for clearing: " + path_);
    }
}

void FileStorage::write(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    const size_t estimated_size = sizeof(uint8_t) + sizeof(uint64_t) + entry.key.size()
        + (entry.isDelete ? 0 : sizeof(uint64_t) + entry.value.size());

    std::vector<char> buffer;
    buffer.reserve(estimated_size);

    uint8_t flag = entry.isDelete ? 1 : 0;
    buffer.push_back(static_cast<char>(flag));

    uint64_t ksize = entry.key.size();
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&ksize),
                  reinterpret_cast<const char*>(&ksize) + sizeof(ksize));
    buffer.insert(buffer.end(), entry.key.begin(), entry.key.end());

    if (!flag) {
        uint64_t vsize = entry.value.size();
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&vsize),
                      reinterpret_cast<const char*>(&vsize) + sizeof(vsize));
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(entry.value.data()),
                      reinterpret_cast<const char*>(entry.value.data() + vsize));
    }

    std::ofstream ofs(path_, std::ios::binary | std::ios::app);
    if (!ofs.is_open()) {
        throw std::runtime_error("FileStorage: failed to open file for writing: " + path_);
    }
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    try {
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        ofs.flush();
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error(
            "FileStorage: write failed for entry with key: " + entry.key + " (" + e.what() + ").");
    }
}

void FileStorage::replay(std::function<void(const LogEntry&)> callback) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream ifs(path_, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("FileStorage: failed to open file for replay: " + path_);
    }
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        while (ifs.peek() != EOF) {
            LogEntry entry;

            uint8_t flag;
            ifs.read(reinterpret_cast<char*>(&flag), sizeof(flag));
            entry.isDelete = (flag != 0);

            uint64_t ksize;
            ifs.read(reinterpret_cast<char*>(&ksize), sizeof(ksize));
            entry.key.resize(ksize);
            ifs.read(entry.key.data(), static_cast<std::streamsize>(ksize));

            if (!entry.isDelete) {
                uint64_t vsize;
                ifs.read(reinterpret_cast<char*>(&vsize), sizeof(vsize));
                entry.value.resize(vsize);
                ifs.read(reinterpret_cast<char*>(entry.value.data()),
                         static_cast<std::streamsize>(vsize));
            }
            callback(entry);
        }
    } catch (const std::ios_base::failure&) {
        if (!ifs.eof()) {
            throw std::runtime_error("FileStorage: replay failed for file: " + path_);
        }
    }
}
