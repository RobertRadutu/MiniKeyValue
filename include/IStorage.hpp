#ifndef __STORAGE_HPP
#define __STORAGE_HPP

#include "LogEntry.hpp"
#include <functional>
class IStorage {
public:
    virtual ~IStorage() = default;
    virtual void clear() = 0;
    virtual void write(const LogEntry& e) = 0;
    virtual void replay(std::function<void(const LogEntry&)>) const = 0;
};

#endif // __STORAGE_HPP