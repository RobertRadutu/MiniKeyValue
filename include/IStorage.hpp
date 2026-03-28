#ifndef MINIKVALUE_ISTORAGE_HPP
#define MINIKVALUE_ISTORAGE_HPP

#include "LogEntry.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual void clear() = 0;
    virtual void write(const LogEntry& e) = 0;
    virtual void replay(std::function<void(const LogEntry&)> callback) const = 0;

    /// Current on-disk size of the log file (bytes). Used for compaction heuristics.
    virtual std::uint64_t sizeBytes() const = 0;

    /// Replace the log with a compacted sequence of live entries only (no tombstones).
    /// Must be atomic from the caller's perspective: on failure, previous log remains valid.
    virtual void compactFromSnapshot(const std::unordered_map<std::string, std::vector<uint8_t>>& live) = 0;
};

#endif // MINIKVALUE_ISTORAGE_HPP
