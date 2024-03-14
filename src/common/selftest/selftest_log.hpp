/**
 * @file selftest_log.hpp
 * @author Radek Vana
 * @brief Workaround for logging to work inside selftest namespace
 * @date 2021-10-12

 */
#pragma once
#include "log.h"
#include "timing.h"
#include <optional>

namespace selftest {
class LogTimer {
    std::optional<uint32_t> last_log;
    uint32_t log_minimal_delay;

public:
    constexpr LogTimer(uint32_t delay)
        : last_log(std::nullopt)
        , log_minimal_delay(delay) {}
    bool CanLog() {
        uint32_t now = ticks_ms();

        if (!last_log.has_value() || (now - *last_log) >= log_minimal_delay) {
            last_log = now;
            return true;
        }
        return false;
    }
    void ForceNextLog() { last_log = std::nullopt; }
};

/**
 * @brief Info log which will be skipped if delay requirements are not met
 * first parameter must be LogTimer object
 */
#define LogInfoTimed(LOG, ...)               \
    {                                        \
        if (LOG.CanLog()) {                  \
            log_info(Selftest, __VA_ARGS__); \
        }                                    \
    }

/**
 * @brief Debug log which will be skipped if delay requirements are not met
 * first parameter must be LogTimer object
 */
#define LogDebugTimed(LOG, ...)               \
    {                                         \
        if (LOG.CanLog()) {                   \
            log_debug(Selftest, __VA_ARGS__); \
        }                                     \
    }
} // namespace selftest
