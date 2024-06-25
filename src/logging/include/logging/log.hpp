#pragma once

#include <cstdarg>
#include <cstdint>

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

/// Log Component representing a source for log events
struct Component {
    /// Name of the log component
    const char *name;

    /// Lowest severity to be logged from this component
    Severity lowest_severity;
};

/// Represents recorded log event
struct Event {
    /// Timestamp of when the event happened
    Timestamp timestamp;

    /// Id of the task which generated the event
    TaskId task_id;

    /// The component which generated the event
    const Component *component;

    /// Severity of the event
    Severity severity;

    /// Format string for the message of the event
    const char *fmt;

    /// Variadic arguments to be used together with the format string above
    /// forming the event's message
    va_list *args;
};

struct FormattedEvent {
    /// Timestamp of when the event happened
    Timestamp timestamp;

    /// Id of the task which generated the event
    TaskId task_id;

    /// The component which generated the event
    const Component *component;

    /// Severity of the event
    Severity severity;

    /// The message of the event
    const char *message;
};

/// Destination (sink) for log events
///
/// Destination is a target to which recorded log events are sent
/// (might be something like the terminal, file, syslog, etc)
struct Destination {
    /// Lowest log severity to be received by this destination
    Severity lowest_severity;

    using LogEventFunction = void(FormattedEvent *event);
    /// The entrypoint for incoming log events
    LogEventFunction *log_event_fn;

    Destination *next;
};

/// Low-level function for recording events.
///
/// Do not use directly if not really needed. Use log_info/log_error/etc defined below.
void __attribute__((format(__printf__, 3, 4)))
_log_event(Severity severity, const Component *component, const char *fmt, ...);

void _log_event_valist(Severity severity, const Component *component, const char *fmt, va_list *args);

/// Find log component for given name
Component *log_component_find(const char *name);

/// Register a new destination to send log events to
void log_destination_register(Destination *destination);

/// Unregister previously registered log destination from receiving log events
void log_destination_unregister(Destination *destination);

#if !defined(LOG_LOWEST_SEVERITY)
    /// \def LOG_LOWEST_SEVERITY
    /// Defines the lowest severity to log in general
    ///
    /// For example if defined as 2 (`INFO`), all `log_debug` calls will be removed at
    /// compile time (saving memory and cpu time at runtime).
    #ifdef _DEBUG
        #define LOG_LOWEST_SEVERITY 1 // DEBUG
    #else
        #define LOG_LOWEST_SEVERITY 2 // INFO
    #endif
#endif

#if LOG_LOWEST_SEVERITY < 1 || LOG_LOWEST_SEVERITY > 5
    #error Invalid LOG_LOWEST_SEVERITY
#endif

#if __APPLE__
    #define _LOG_COMPONENT_ATTRS __attribute__((used, section("__DATA,log_components")))
#elif !defined(__arm__)
    #define _LOG_COMPONENT_ATTRS __attribute__((used, section("log_components")))
#else
    #define _LOG_COMPONENT_ATTRS __attribute__((used, section(".data.log_components")))
#endif

/// \def LOG_COMPONENT(name)
/// (internal use only)
/// Return the name of the underlying Component structure for given component name
#define LOG_COMPONENT(name) __log_component_##name

/// \def LOG_COMPONENT_DEF(name, default_severity)
/// Define a new log component
///
/// A component defined using this macro can be (without any further registration at
/// runtime) discovered by the logging subsystem.
/// How does it work? It places the definition (the Component structure) to a
/// separate section in memory called .data.log_components. The linker file defines start and
/// end symbols for this section (see linker scripts). Those start/end symbols are used
/// to find the block of memory with all the component definitions at runtime.
///
/// Usage:
///
///    LOG_COMPONENT_DEF(MyComponent, logging::Severity::warning);
///
#define LOG_COMPONENT_DEF(name, default_severity) \
    logging::Component LOG_COMPONENT(name) _LOG_COMPONENT_ATTRS = { #name, default_severity }

/// \def log_event(severity, component, fmt, ...)
/// Record a log event
///
/// To be used when the severity isn't known at compile time.
///
/// Usage:
///
///    log_event(severity_variable, MyComponent, "Something has happened");
///

#define log_event(severity, component, fmt, ...)                                \
    do {                                                                        \
        _log_event(severity, &__log_component_##component, fmt, ##__VA_ARGS__); \
    } while (0)

/// \def LOG_COMPONENT_REF(component)
/// References an existing component
///
/// To be used when logging to a component defined in another file.
///
/// Usage (top of the file):
///    LOG_COMPONENT_REF(MyComponent);
///
#define LOG_COMPONENT_REF(component) extern logging::Component LOG_COMPONENT(component)

/// \def log_debug(component, fmt, ...)
/// Record a log event with `debug` severity.
#if LOG_LOWEST_SEVERITY <= 1
    #define log_debug(component, fmt, ...) log_event(logging::Severity::debug, component, fmt, ##__VA_ARGS__)
#else
    #define log_debug(component, fmt, ...)
#endif

/// \def log_info(component, fmt, ...)
/// Record a log event with `info` severity.
#if LOG_LOWEST_SEVERITY <= 2
    #define log_info(component, fmt, ...) log_event(logging::Severity::info, component, fmt, ##__VA_ARGS__)
#else
    #define log_info(component, fmt, ...)
#endif

/// \def log_warning(component, fmt, ...)
/// Record a log event with `warning` severity.
#if LOG_LOWEST_SEVERITY <= 3
    #define log_warning(component, fmt, ...) log_event(logging::Severity::warning, component, fmt, ##__VA_ARGS__)
#else
    #define log_warning(component, fmt, ...)
#endif

/// \def log_error(component, fmt, ...)
/// Record a log event with `error` severity.
#if LOG_LOWEST_SEVERITY <= 4
    #define log_error(component, fmt, ...) log_event(logging::Severity::error, component, fmt, ##__VA_ARGS__)
#else
    #define log_error(component, fmt, ...)
#endif

/// \def log_critical(component, fmt, ...)
/// Record a log event with `critical` severity.
#if LOG_LOWEST_SEVERITY <= 5
    #define log_critical(component, fmt, ...) log_event(logging::Severity::critical, component, fmt, ##__VA_ARGS__)
#else
    #define log_critical(component, fmt, ...)
#endif

void log_task_process_event(FormattedEvent *event);

} // namespace logging
