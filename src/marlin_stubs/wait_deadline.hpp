/// @file
/// Timeout bookkeeping for blocking wait G-codes (M269, ...).
/// No hardware dependencies - covered by unit tests in tests/unit/feature/io_expander.
#pragma once

#include <cstdint>
#include <optional>

/// Uses unsigned arithmetic, so it is immune to the millisecond tick counter wrapping around.
class WaitDeadline {
public:
    WaitDeadline(uint32_t now_ms, std::optional<uint32_t> timeout_ms)
        : start_ms(now_ms)
        , timeout_ms(timeout_ms) {}

    bool expired(uint32_t now_ms) const {
        return timeout_ms.has_value() && (now_ms - start_ms) >= *timeout_ms;
    }

private:
    uint32_t start_ms;
    std::optional<uint32_t> timeout_ms;
};
