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

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual void clear() = 0;
    virtual void write(const std::string& key, const std::vector<uint8_t>& value, bool isDelete) = 0;
    virtual void readAll(std::unordered_map<std::string, std::vector<uint8_t>>& store) const = 0;
};

class FileStorage : public IStorage {
public:
    explicit FileStorage(const std::string& path) : path_(path) {}

    void clear() override {
        std::ofstream ofs(path_, std::ios::binary | std::ios::trunc);
    }

    void write(const std::string& key, const std::vector<uint8_t>& value, bool isDelete) override {
        std::ofstream ofs{path_, std::ios::binary | std::ios::app};
        uint8_t flag = isDelete ? 1 : 0;
        ofs.write(reinterpret_cast<char*>(&flag), sizeof(flag));

        uint64_t ksize = key.size();
        ofs.write(reinterpret_cast<const char*>(&ksize), sizeof(ksize));
        ofs.write(key.data(), ksize);

        if(!flag){
            uint64_t vsize = value.size();
            ofs.write(reinterpret_cast<const char*>(&vsize), sizeof(vsize));
            ofs.write(reinterpret_cast<const char*>(value.data()), vsize);
        }
        ofs.flush();
    }

    void readAll(std::unordered_map<std::string, std::vector<uint8_t>>& store) const override {
        std::ifstream ifs(path_, std::ios::binary);
        if(!ifs){
            std::runtime_error("Couldn't open logFile!\n");
            return;
        }

        while(true) {
            uint8_t flag;
            if(!ifs.read(reinterpret_cast<char*>(&flag), sizeof(flag))) break;
            bool isDelete = flag == 1;

            uint64_t ksize;
            if(!ifs.read(reinterpret_cast<char*>(&ksize), sizeof(ksize))) break;
            std::string key(ksize, '\0');
            if(!ifs.read(key.data(), ksize)) break;

            if(!isDelete) {
                uint64_t vsize;
                if(!ifs.read(reinterpret_cast<char*>(&vsize), sizeof(vsize))) break;
                std::vector<uint8_t> value(vsize);
                if(!ifs.read(reinterpret_cast<char*>(value.data()), vsize)) break;
                store[key] = std::move(value);
            }
            else {
                store.erase(key);
            }
        }

    }
private:
    std::string path_;
};

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
        storage_->readAll(store);
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