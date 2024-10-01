#pragma once
#include <stdint.h>
#include <stddef.h>

namespace MMU2 {

/// A minimal serial interface for the MMU
class MMU2Serial {
public:
    MMU2Serial() = default;
    void begin();
    void close();
    int read();
    void flush();
    size_t write(const uint8_t *buffer, size_t size);

    /// Checks if serial recovery is necessary and potentially performs it
    void check_recovery();

private:
    uint32_t recovery_start_ms = 0;
};

extern MMU2Serial mmu2Serial;

} // namespace MMU2
