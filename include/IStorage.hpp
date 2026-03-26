#ifndef MINIKVALUE_ISTORAGE_HPP
#define MINIKVALUE_ISTORAGE_HPP

#include "LogEntry.hpp"
#include <functional>

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual void clear() = 0;
    virtual void write(const LogEntry& e) = 0;
    virtual void replay(std::function<void(const LogEntry&)> callback) const = 0;
};

#endif // MINIKVALUE_ISTORAGE_HPP
