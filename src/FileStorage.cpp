#include "../include/FileStorage.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

void serializeEntryToBuffer(const LogEntry& entry, std::vector<char>& buffer) {
    const size_t estimated_size = sizeof(uint8_t) + sizeof(uint64_t) + entry.key.size()
        + (entry.isDelete ? 0 : sizeof(uint64_t) + entry.value.size());
    buffer.clear();
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
}

void writeBufferToAppendFile(const std::string& path, const std::vector<char>& buffer) {
    std::ofstream ofs(path, std::ios::binary | std::ios::app);
    if (!ofs.is_open()) {
        throw std::runtime_error("FileStorage: failed to open file for writing: " + path);
    }
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    ofs.flush();
}

} // namespace

void FileStorage::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream ofs(path_, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        throw std::runtime_error("FileStorage: failed to open file for clearing: " + path_);
    }
}

void FileStorage::write(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<char> buffer;
    serializeEntryToBuffer(entry, buffer);
    writeBufferToAppendFile(path_, buffer);
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

std::uint64_t FileStorage::sizeBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::error_code ec;
    const auto sz = std::filesystem::file_size(path_, ec);
    if (ec) {
        return 0;
    }
    return static_cast<std::uint64_t>(sz);
}

void FileStorage::compactFromSnapshot(const std::unordered_map<std::string, std::vector<uint8_t>>& live) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string tmp_path = path_ + ".compact.tmp";
    std::error_code rm_ec;
    std::filesystem::remove(tmp_path, rm_ec);

    {
        std::ofstream ofs(tmp_path, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            throw std::runtime_error("FileStorage: failed to open temp file for compaction: " + tmp_path);
        }
        ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        std::vector<char> buffer;
        for (const auto& kv : live) {
            LogEntry e{kv.first, kv.second, false};
            serializeEntryToBuffer(e, buffer);
            ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        }
        ofs.flush();
    }

    /**
        This is not atomic in the sense that is implemented with a single CPU instruction,
        but the fact that other threads will either see the old file or new file at path_,
        after the following code
    */
    std::error_code rename_ec;
    std::filesystem::rename(tmp_path, path_, rename_ec);
    if (rename_ec) {
        std::filesystem::remove(tmp_path, rm_ec);
        throw std::runtime_error("FileStorage: atomic rename after compaction failed: " + path_);
    }
}
