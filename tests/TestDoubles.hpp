#ifndef MINIKVALUE_TEST_DOUBLES_HPP
#define MINIKVALUE_TEST_DOUBLES_HPP

#include "../include/IStorage.hpp"
#include <stdexcept>
#include <vector>

/// Fake storage that keeps the append-only log entirely in memory.
/// Behaves identically to FileStorage from MiniKeyValue's perspective,
/// but with zero filesystem interaction.
class InMemoryStorage : public IStorage {
public:
    void clear() override { entries_.clear(); }

    void write(const LogEntry& e) override { entries_.push_back(e); }

    void write(const std::vector<LogEntry>& entries) override {
        entries_.insert(entries_.end(), entries.begin(), entries.end());
    }

    void replay(std::function<void(const LogEntry&)> callback) const override {
        for (const auto& entry : entries_) {
            callback(entry);
        }
    }

    std::uint64_t sizeBytes() const override {
        std::uint64_t n = 0;
        for (const auto& e : entries_) {
            n += sizeof(uint8_t) + sizeof(uint64_t) + e.key.size();
            if (!e.isDelete) {
                n += sizeof(uint64_t) + e.value.size();
            }
        }
        return n;
    }

    void compactFromSnapshot(
        const std::unordered_map<std::string, std::vector<uint8_t>>& live) override {
        entries_.clear();
        for (const auto& [key, value] : live) {
            entries_.push_back({key, value, false});
        }
    }

    const std::vector<LogEntry>& entries() const { return entries_; }

private:
    std::vector<LogEntry> entries_;
};

/// Saboteur storage that throws on every write.
/// All other operations are harmless no-ops so the object can be constructed
/// and injected without side-effects before the first mutation.
class FailingStorage : public IStorage {
public:
    void clear() override {}

    void write(const LogEntry&) override {
        throw std::runtime_error("FailingStorage: simulated write failure");
    }

    void write(const std::vector<LogEntry>&) override {
        throw std::runtime_error("FailingStorage: simulated batch write failure");
    }

    void replay(std::function<void(const LogEntry&)>) const override {}

    std::uint64_t sizeBytes() const override { return 0; }

    void compactFromSnapshot(
        const std::unordered_map<std::string, std::vector<uint8_t>>&) override {}
};

#endif // MINIKVALUE_TEST_DOUBLES_HPP
