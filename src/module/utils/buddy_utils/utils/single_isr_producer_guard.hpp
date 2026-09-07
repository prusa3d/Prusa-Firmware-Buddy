#pragma once

#include <cstdint>

#include <bsod/bsod.h>

/// Debug helper for single-producer resources (e.g. an SPSC AtomicCircularQueue).
/// Keep one static instance per resource and call check() on each use;
/// BSOD on multiple ISR producers.
class SingleISRProducerGuard {
#ifndef NDEBUG
    static uint32_t exception_number() {
    #if defined(__arm__) || defined(__ARM_ARCH)
        uint32_t ipsr;
        __asm volatile("mrs %0, ipsr"
                       : "=r"(ipsr));
        return ipsr; // 0 == thread mode, non-zero == the active ISR
    #else
        #error
    #endif
    }

    static constexpr uint32_t unset = 0xFFFFFFFF;
    uint32_t producer = unset;

public:
    void check() {
        const uint32_t here = exception_number();
        if (producer == unset) {
            producer = here;
        } else {
            if (producer != here) {
                bsod("Multiple ISR producers encountered!");
            }
        }
    }
#else
public:
    void check() {}
#endif
};
