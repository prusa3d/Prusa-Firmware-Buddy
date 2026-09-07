/// @file
#pragma once

#include <span>
#include <utils/byte_utils.hpp>
#include <cstdint>
#include <cstddef>

#include <utils/uncopyable.hpp>

/// Class for checking stack overflows
/// ! Consider using a different mechanism if the platform supports it, for example MSPLIM
class StackOverflowChecker : public Uncopyable {

public:
    constexpr inline StackOverflowChecker(const WritableBytes &stack)
        : stack_start(reinterpret_cast<uint32_t *>(stack.data())) {

        // Write our magic constant to the beginning (= bottom) of the stack. If it gets overwritten, we trigger an alarm
        *stack_start = magic_constant;
    }

    /// \returns true if a stack overflow was detected
    [[nodiscard]] inline bool has_overflowed() const {
        return *stack_start != magic_constant;
    }

    /// \returns a memory region the checker needs to be able to read from for the \p has_overflowed check
    /// This is necessary to consider for boards with the MPU enabled
    inline Bytes read_access_region() const {
        return std::as_bytes(std::span { stack_start, stack_start + 1 });
    }

private:
    static constexpr uint32_t magic_constant = 0xdeadbeef;
    uint32_t *stack_start;
};
