// Unittest stub

#pragma once

namespace freertos {

class Mutex {
public:
    static thread_local int locked_mutex_count;

    void lock();
    void unlock();
};

} // namespace freertos
