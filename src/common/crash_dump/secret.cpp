#include "secret.hpp"
#include <bsod/bsod.h>

#ifndef UNITTESTS
    #include <buddy/memory.hpp>
#endif

namespace crash_dump {

void PrivacyProtection::reg(void *ptr, size_t size) {
    std::unique_lock lock(mutex);
    for (auto &record : to_delete) {
        if (!record.has_value()) {
            record = { ptr, size };
            return;
        }
    }
    debug_assert(false);
}

void PrivacyProtection::unreg(void *ptr) {
    std::unique_lock lock(mutex);
    for (auto &record_opt : to_delete) {
        if (record_opt.has_value()) {
            auto record = record_opt.value();
            if (record.ptr == ptr) {
                record_opt = std::nullopt;
                return;
            }
        }
    }
    debug_assert(false);
}

// Intentionally not locking, scheduler is already disabled at this point
void PrivacyProtection::clean_up() {
    for (auto &record_opt : to_delete) {
        if (record_opt.has_value()) {
            auto record = record_opt.value();
#ifndef UNITTESTS
            // Make sure we are zeroing valid memory, in case the pointers
            // would be overwritten by some kind of overflow, we are in BSOD
            // after all
            if (!is_ram(reinterpret_cast<uintptr_t>(record.ptr))) {
                continue;
            }
#endif
            memset(record.ptr, 0, record.size);
        }
    }
}

PrivacyProtection privacy_protection;

ManualSecret::ManualSecret(void *address, size_t size)
    : address(address) {
    privacy_protection.reg(address, size);
}

ManualSecret::~ManualSecret() {
    privacy_protection.unreg(address);
}
} // namespace crash_dump
