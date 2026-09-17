#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

/// Hands out consecutive chunks of an in-memory buffer, refusing to run past
/// its end.
class BufferReader {
    std::span<const uint8_t> remaining;

public:
    explicit BufferReader(std::span<const uint8_t> data)
        : remaining(data) {}

    /// Advance past `n` bytes and return a pointer to their start, or nullptr
    /// if fewer than `n` bytes remain.
    const uint8_t *read(size_t n) {
        if (n > remaining.size()) {
            return nullptr;
        }
        const uint8_t *chunk = remaining.data();
        remaining = remaining.subspan(n);
        return chunk;
    }
};
