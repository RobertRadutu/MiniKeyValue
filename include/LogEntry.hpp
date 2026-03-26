#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct LogEntry {
    std::string key;
    std::vector<uint8_t> value;
    bool isDelete{false};
};
