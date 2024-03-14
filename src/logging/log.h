#pragma once
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Timestamp from the startup
typedef struct {
    uint32_t sec; ///< Seconds since the start of the system
    uint32_t us; ///< Microseconds consistent with sec
} log_timestamp_t;

/// Task identifier (-1 if unknown)
typedef int log_task_id_t;

/// Severity of a log event
typedef enum {
    LOG_SEVERITY_DEBUG = 1,
    LOG_SEVERITY_INFO = 2,
    LOG_SEVERITY_WARNING = 3,
    LOG_SEVERITY_ERROR = 4,
    LOG_SEVERITY_CRITICAL = 5
} log_severity_t;

/// Log Component representing a source for log events
typedef struct {
    /// Name of the log component
    const char *name;

    /// Lowest severity to be logged from this component
    log_severity_t lowest_severity;
} log_component_t;

/// Represents recorded log event
typedef struct {
    /// Timestamp of when the event happened
    log_timestamp_t timestamp;

    /// Id of the task which generated the event
    log_task_id_t task_id;

    /// The component which generated the event
    const log_component_t *component;

    /// Severity of the event
    log_severity_t severity;

    /// Format string for the message of the event
    const char *fmt;

    /// Variadic arguments to be used together with the format string above
    /// forming the event's message
    va_list *args;
} log_event_t;

/// Destination (sink) for log events
///
/// Destination is a target to which recorded log events are sent
/// (might be something like the terminal, file, syslog, etc)
typedef struct log_destination_s {
    /// Name of the destination
    const char *name;

    /// Lowest log severity to be received by this destination
    log_severity_t lowest_severity;

    /// The entrypoint for incoming log events
    void (*log_event_fn)(struct log_destination_s *destination, log_event_t *event);

    /// Formatting function the destination should when serializing log events
    void (*log_format_fn)(log_event_t *event, void (*out_fn)(char character, void *arg), void *arg);
    struct log_destination_s *next;
} log_destination_t;

/// Low-level function for recording events.
///
/// Do not use directly if not really needed. Use log_info/log_error/etc defined below.
void _log_event(log_severity_t severity, const log_component_t *component, const char *fmt, ...);

/// Find log component for given name
log_component_t *log_component_find(const char *name);

/// Register a new destination to send log events to
void log_destination_register(log_destination_t *destination);

/// Find a previously registered log destination
log_destination_t *log_destination_find(const char *name);

/// Unregister previously registered log destination from receiving log events
void log_destination_unregister(log_destination_t *destination);

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
/// Return the name of the underlying log_component_t structure for given component name
#define LOG_COMPONENT(name) __log_component_##name

/// \def LOG_COMPONENT_DEF(name, default_severity)
/// Define a new log component
///
/// A component defined using this macro can be (without any further registration at
/// runtime) discovered by the logging subsystem.
/// How does it work? It places the definition (the log_component_t structure) to a
/// separate section in memory called .data.log_components. The linker file defines start and
/// end symbols for this section (see linker scripts). Those start/end symbols are used
/// to find the block of memory with all the component definitions at runtime.
///
/// Usage:
///
///    LOG_COMPONENT_DEF(MyComponent, LOG_SEVERITY_WARNING);
///
#define LOG_COMPONENT_DEF(name, default_severity) \
    log_component_t LOG_COMPONENT(name) _LOG_COMPONENT_ATTRS = { #name, default_severity }

/// \def log_event(severity, component, fmt, ...)
/// Record a log event
///
/// To be used when the severity isn't known at compile time.
///
/// Usage:
///
///    log_event(severity_variable, MyComponent, "Something has happened");
///
#ifdef __cplusplus
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
    #define LOG_COMPONENT_REF(component) extern log_component_t LOG_COMPONENT(component)
#else
    #define log_event(severity, component, fmt, ...)                                \
        do {                                                                        \
            extern log_component_t LOG_COMPONENT(component);                        \
            _log_event(severity, &__log_component_##component, fmt, ##__VA_ARGS__); \
        } while (0)
#endif

/// \def log_debug(component, fmt, ...)
/// Record a log event with `debug` severity.
#if LOG_LOWEST_SEVERITY <= 1
    #define log_debug(component, fmt, ...) log_event(LOG_SEVERITY_DEBUG, component, fmt, ##__VA_ARGS__)
#else
    #define log_debug(component, fmt, ...)
#endif

/// \def log_info(component, fmt, ...)
/// Record a log event with `info` severity.
#if LOG_LOWEST_SEVERITY <= 2
    #define log_info(component, fmt, ...) log_event(LOG_SEVERITY_INFO, component, fmt, ##__VA_ARGS__)
#else
    #define log_info(component, fmt, ...)
#endif

/// \def log_warning(component, fmt, ...)
/// Record a log event with `warning` severity.
#if LOG_LOWEST_SEVERITY <= 3
    #define log_warning(component, fmt, ...) log_event(LOG_SEVERITY_WARNING, component, fmt, ##__VA_ARGS__)
#else
    #define log_warning(component, fmt, ...)
#endif

/// \def log_error(component, fmt, ...)
/// Record a log event with `error` severity.
#if LOG_LOWEST_SEVERITY <= 4
    #define log_error(component, fmt, ...) log_event(LOG_SEVERITY_ERROR, component, fmt, ##__VA_ARGS__)
#else
    #define log_error(component, fmt, ...)
#endif

/// \def log_critical(component, fmt, ...)
/// Record a log event with `critical` severity.
#if LOG_LOWEST_SEVERITY <= 5
    #define log_critical(component, fmt, ...) log_event(LOG_SEVERITY_CRITICAL, component, fmt, ##__VA_ARGS__)
#else
    #define log_critical(component, fmt, ...)
#endif

#ifdef __cplusplus
}
#endif //__cplusplus
