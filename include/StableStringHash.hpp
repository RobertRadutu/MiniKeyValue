#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

/// FNV-1a 64-bit hash — stable across platforms for a given string (unlike
/// std::hash<std::string>). Used to pick a shard index for distributed routing.
inline std::uint64_t stableStringHash(std::string_view s) {
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= static_cast<std::uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

/// Shard index in [0, shard_count) for consistent sharding across clients and servers.
inline std::size_t shardIndexForKey(std::string_view key, std::size_t shard_count) {
    if (shard_count == 0) {
        return 0;
    }
    return static_cast<std::size_t>(stableStringHash(key) % shard_count);
}
