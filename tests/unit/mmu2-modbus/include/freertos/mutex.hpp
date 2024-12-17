#pragma once

namespace freertos {

class Mutex {
public:
public:
    Mutex() {};
    ~Mutex() {};
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;
    void unlock() {}
    bool try_lock() { return true; }
    void lock() {}
};

} // namespace freertos
