#include "../include/MiniKeyValue.hpp"

std::vector<uint8_t> MiniKeyValue::Get(const std::string& key) const{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = store.find(key);
    if(it != store.end()) return it->second;
    return {};
}

void MiniKeyValue::Set(const std::string& key, const std::vector<uint8_t>& value){
    std::lock_guard<std::mutex> lock{mtx};
    store[key] = value;
    
    LogEntry entry{key, value, false};
    storage_->write(entry);
}

void MiniKeyValue::Delete(const std::string& key){
    std::lock_guard<std::mutex> lock{mtx};
    store.erase(key);

    LogEntry entry{key, {}, true};
    storage_->write(entry);
}

// std::vector<std::vector<uint8_t>> MiniKeyValue::GetAllData() const{
//     std::vector<std::vector<uint8_t>> allData;
//     for(const auto& [key, value] : store){
//         allData.push_back(value);
//     }
//     return allData;
// }

bool MiniKeyValue::isKey(const std::string& key) const{
    std::lock_guard<std::mutex> lock{mtx};
    return store.count(key) != 0 ? true : false;
}

void MiniKeyValue::resetFile() {
    std::ofstream ofs{logFile, std::ios::binary | std::ios::trunc};
}

