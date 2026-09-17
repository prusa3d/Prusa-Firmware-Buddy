/// @file RAM-backed simulation of a NOR flash memory for host unit tests.
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace mock {

/// Erase sets a whole block to 0xFF, program can only clear bits, and both
/// must stay in range. Breaking a rule throws std::logic_error, which Catch2
/// reports as a failed test. Operations can be made to fail through
/// inject_errors().
template <uint32_t Size, uint32_t EraseBlockSize>
class FlashSim {
    static_assert(Size % EraseBlockSize == 0);

public:
    FlashSim() { reset(); }

    /// Returns false on an injected error; \p data then holds deterministic garbage.
    bool read(uint32_t addr, uint8_t *data, uint32_t len) {
        check_range(addr, len);
        if (take_injected_error()) {
            memset(data, 0xEE, len);
            return false;
        }
        memcpy(data, &memory[addr], len);
        return true;
    }

    /// Returns false on an injected error
    bool program(uint32_t addr, const uint8_t *data, uint32_t len) {
        check_range(addr, len);
        if (take_injected_error()) {
            return false;
        }
        for (uint32_t i = 0; i < len; ++i) {
            uint8_t &cell = memory[addr + i];
            cell &= data[i];

            if (data[i] != cell) {
                throw std::logic_error("FlashSim: programmed value not read back, block not erased");
            }
        }
        return true;
    }

    /// Erase one block
    /// \p addr must be block-aligned.
    /// Returns false on an injected error
    bool erase_block(uint32_t addr) {
        check_range(addr, EraseBlockSize);
        if (addr % EraseBlockSize != 0) {
            throw std::logic_error("FlashSim: unaligned erase");
        }
        if (take_injected_error()) {
            return false;
        }
        memset(&memory[addr], 0xFF, EraseBlockSize);
        return true;
    }

    // --- test control ---

    /// Erase everything and forget injected errors
    void reset() {
        memory.fill(0xFF);
        errors_to_inject = 0;
    }

    /// Make the next \p count operations fail
    void inject_errors(int count = 1) { errors_to_inject = count; }

    /// Direct access bypassing the NOR rules, e.g. to simulate a torn write
    uint8_t &byte(uint32_t addr) { return memory.at(addr); }

private:
    void check_range(uint32_t addr, uint32_t len) const {
        if (uint64_t(addr) + len > Size) {
            throw std::logic_error("FlashSim: access out of range");
        }
    }

    bool take_injected_error() {
        if (errors_to_inject <= 0) {
            return false;
        }
        --errors_to_inject;
        return true;
    }

    std::array<uint8_t, Size> memory;
    int errors_to_inject = 0;
};

} // namespace mock
