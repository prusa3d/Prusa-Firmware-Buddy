#pragma once

#include <cstdint>

#define log_debug(...)
#define log_info(...)
#define log_error(...)
#define log_warning(...)
#define log_critical(...)

#define LOG_COMPONENT_DEF(...)
#define LOG_COMPONENT_REF(...)

namespace logging {

/// Timestamp from the startup
struct Timestamp {
    uint32_t sec; ///< Seconds since the start of the system
    uint32_t us; ///< Microseconds consistent with sec
};

/// Task identifier (-1 if unknown)
using TaskId = int;

/// Severity of a log event
enum class Severity {
    debug = 1,
    info = 2,
    warning = 3,
    error = 4,
    critical = 5
};

/// Represents recorded log event
struct Event {
    /// Timestamp of when the event happened
    Timestamp timestamp;
};

struct FormattedEvent {
    /// Timestamp of when the event happened
    Timestamp timestamp;

    /// Severity of the event
    Severity severity;
};
} // namespace logging
