#pragma once
#include <string>
#include <vector>
#include <cstdint>

/**
 * POD -> Plain Old Data, no behavior related, no OOP
 */
struct LogEntry {
    std::string key;
    std::vector<uint8_t> value;
    bool isDelete{false};
};