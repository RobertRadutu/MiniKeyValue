#ifndef MINIKVALUE_MINIKEYVALUE_HPP
#define MINIKVALUE_MINIKEYVALUE_HPP

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

class IStorage;

class MiniKeyValue final {
    struct FactoryAccess {
        friend class MiniKeyValue;
    private:
        FactoryAccess() = default;
    };

public:
    explicit MiniKeyValue(std::unique_ptr<IStorage> storage, FactoryAccess);
    static std::shared_ptr<MiniKeyValue> createEmpty(const std::string& path);
    static std::shared_ptr<MiniKeyValue> createMiniKeyValue(const std::string& path);
    std::optional<std::vector<uint8_t>> Get(const std::string& key) const;
    void Set(const std::string& key, const std::vector<uint8_t>& value);
    void Delete(const std::string& key);
    std::unordered_map<std::string, std::vector<uint8_t>> GetAllKeyValues() const;

    ~MiniKeyValue();
    MiniKeyValue(const MiniKeyValue&) = delete;
    MiniKeyValue& operator=(const MiniKeyValue&) = delete;
    MiniKeyValue(MiniKeyValue&&) = delete;
    MiniKeyValue& operator=(MiniKeyValue&&) = delete;

private:
    void readFile();

    std::unique_ptr<IStorage> storage_;
    std::unordered_map<std::string, std::vector<uint8_t>> store_;
    mutable std::mutex mtx_;
};

#endif // MINIKVALUE_MINIKEYVALUE_HPP
