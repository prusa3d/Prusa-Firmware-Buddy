#include <common/freertos_mutex.hpp>

// uncomment this to get buffer size ass compile time error message
// char (*__give_me_buffere_size)[sizeof( StaticSemaphore_t )] = 1;
// use that size in header as buffer_size

namespace freertos {
void Mutex::unlock() {}
bool Mutex::try_lock() { return true; }
void Mutex::lock() { return; }
Mutex::Mutex() noexcept // ctor should be constexpr, but cannot due C code
    : xSemaphore(nullptr) {};
} // namespace freertos
void buddy::lock(std::unique_lock<freertos::Mutex> &l1, std::unique_lock<freertos::Mutex> &l2) {
    if (&l1 < &l2) {
        l1.lock();
        l2.lock();
    } else {
        l2.lock();
        l1.lock();
    }
}
