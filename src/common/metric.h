#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> //NULL
#include <timing.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

///
/// # Metrics
///
/// What is it?
/// Let's say you want to fine-tune some real-time algorithm - metrics allow
/// you to easily define the values you care about and observe them
/// in real-time while your algorithm runs on a real printer.
///
/// Quick start
/// 1. Define your metrics (or use an existing one)
///
///    METRIC_DEF(val_xyz, "val_xyz", METRIC_VALUE_INTEGER, 100, METRIC_HANDLER_DISABLE_ALL);
///
///     - The first parameter - "val_xyz" - is the metric's name. Keep it as short as possible!
///     - The second parameter defines the type of recorded points (values) of this metric.
///     - The third parameter - `100` - defines the minimal interval between consecutive recorded points in ms.
///          - E.g. the value 100 ms makes the `val_xyz` metric being transmitted at maximum frequency 10 Hz.
///          - If you want to disable throttling and send the values as fast as possible, set it to 0.
///     - The last parameter is a bitmap specifying which handlers should have this metric enabled after startup.
///
/// 2. Record your values
///
///    metric_record_integer(&val_xyz, 314);
///
/// 3. Observe!
///
///    TODO: complete those instructions
///

#define METRIC_HANDLER_ENABLE_ALL  (0xff)
#define METRIC_HANDLER_DISABLE_ALL (0x00)

typedef enum {
    METRIC_VALUE_EVENT = 0x00, // no value, just an event
    METRIC_VALUE_FLOAT = 0x01,
    METRIC_VALUE_INTEGER = 0x02,
    METRIC_VALUE_STRING = 0x03,
    METRIC_VALUE_CUSTOM = 0x04, // multiple values formatted via customized line protocol (see metrics.md)
} metric_value_type_t;

/// A metric definition.
///
/// Use the METRIC_DEF(...) macro to define a metric.
typedef struct metric_s {
    /// The name of the metric.
    ///
    /// Keep this short and informative.
    /// It is the unique identifier for the metric.
    const char *const name;

    /// Internal. Use at your own risk.
    uint32_t _last_update_timestamp;

    /// Allows throttling of the recorded values.
    ///
    /// For example, if min_interval_ms is 50, the values
    /// of this metric are not going to be sent faster then
    /// at 20 Hz.
    /// When set to zero, no throttling is going to be performed.
    uint16_t min_interval_ms;

    /// The type of the values associated with this metric.
    const metric_value_type_t type : 8;

    /// Specifies, which handlers are going to receive points for this metric.
    ///
    /// Bitmap, where bit on position METRIC_HANDLER_<NAME>_ID represents, whether
    /// the handler <NAME> should receive points for this metric.
    ///
    /// Set to METRIC_HANDLER_ENABLE_ALL or METRIC_HANDLER_DISABLE_ALL to enable/disable
    /// this metric globally.
    uint8_t enabled_handlers;

} metric_t;

#if __APPLE__
    #define _METRIC_DEF_ATTRS __attribute__((used, section("__DATA,metric_definitions")))
#elif !defined(__arm__)
    #define _METRIC_DEF_ATTRS __attribute__((used, section("metric_definitions")))
#else
    #define _METRIC_DEF_ATTRS __attribute__((used, section(".data.metric_definitions")))
#endif

/// To be used for metric_t structure initialization.
#define METRIC_DEF(var, name, type, min_interval_ms, enabled_handlers) static metric_t var _METRIC_DEF_ATTRS = { name, 0, min_interval_ms, type, enabled_handlers }

/// Represents a single recorded value.
///
/// Do not create this manually. It is created automatically
/// by metric_record_* functions and passed to handlers.
///
/// This is an internal short-lived object with the purpose of transfering
/// a recording of a metric from one task to another.
typedef struct {
    /// The metric for which the value was recorded.
    metric_t *metric;

    /// Timestamp of the metric in us.
    /// Oveflows and we are fine with it. Measures the interval between metric transfers only.
    uint32_t timestamp;

    /// Is it an error message?
    bool error;

    union {
        /// Float value (if metric.type == METRIC_VALUE_FLOAT)
        float value_float;

        /// Integer value (if metric.type == METRIC_VALUE_INTEGER)
        int value_int;

        /// String value (if metric.type == METRIC_VALUE_STRING)
        char value_str[48];

        /// Custom value (if metric.type == METRIC_VALUE_CUSTOM)
        char value_custom[48];

        /// Error message (if recording.error == true)
        char error_msg[48];
    };
} metric_point_t;

/// Metric Handler
///
/// Handler is the entity processing all recorded points. It
/// can send them via UART, aggregate them and create new metrics,
/// or send them via Ethernet. So many options!
typedef struct {
    /// The identifier of this handler.
    uint8_t identifier;

    /// Human-friendly name of the handler.
    const char *name;

    /// The function to be called for every recorded point.
    ///
    /// It is not called if the handler is not enabled (metric_t.enabled_handlers).
    void (*handle_fn)(metric_point_t *point);
} metric_handler_t;

typedef const metric_handler_t *const *metric_handler_list_t;

/// Initialize metrics
///
/// Starts a new lightweight task processing all the recorded metrics.
void metric_system_init();

/// Record a float (metric.type has to be METRIC_VALUE_FLOAT)
void metric_record_float(metric_t *metric, float value);

/// Record a float with given timestamp (metric.type has to be METRIC_VALUE_FLOAT)
void metric_record_float_at_time(metric_t *metric, uint32_t timestamp, float value);

/// Record an integer (metric.type has to be METRIC_VALUE_INTEGER)
void metric_record_integer(metric_t *metric, int value);

/// Record an integer with given timestamp (metric.type has to be METRIC_VALUE_INTEGER)
void metric_record_integer_at_time(metric_t *metric, uint32_t timestamp, int value);

/// Record a string (metric.type has to be METRIC_VALUE_STRING)
///
/// The string is automatically truncated to the length of metric_point_t.value_str buffer size.
void metric_record_string(metric_t *metric, const char *fmt, ...) __attribute__((format(__printf__, 2, 3)));

/// Record a string with given timestamp (metric.type has to be METRIC_VALUE_STRING)
///
/// The string is automatically truncated to the length of metric_point_t.value_str buffer size.
void __attribute__((format(__printf__, 3, 4)))
metric_record_string_at_time(metric_t *metric, uint32_t timestamp, const char *fmt, ...);

/// Record an event (metric.type has to be METRIC_VALUE_EVENT)
void metric_record_event(metric_t *metric);

/// Record an event with given timestamp (metric.type has to be METRIC_VALUE_EVENT)
void metric_record_event_at_time(metric_t *metric, uint32_t timestamp);

/// Record a custom event (metric.type has to be METRIC_VALUE_CUSTOM)
///
/// This is a lower-level function. Improper use can lead to terible things.
/// And nobody wants that, so use only if you know what you are doing.
///
/// A metric error (datapoint with error=<message>) is recorded in case the resulting
/// string does not fit the internal buffers.
void metric_record_custom(metric_t *metric, const char *fmt, ...) __attribute__((format(__printf__, 2, 3)));

/// Record a custom event with given timestamp (metric.type has to be METRIC_VALUE_CUSTOM)
///
/// This is a lower-level function. Improper use can lead to terible things.
/// And nobody wants that, so use only if you know what you are doing.
///
/// A metric error (datapoint with error=<message>) is recorded in case the resulting
/// string does not fit the internal buffers.
void __attribute__((format(__printf__, 3, 4)))
metric_record_custom_at_time(metric_t *metric, uint32_t timestamp, const char *fmt, ...);

/// Records an error for a given metric.
void __attribute__((format(__printf__, 2, 3)))
metric_record_error(metric_t *metric, const char *fmt, ...);

/// Return null-terminated list of handlers
metric_handler_list_t metric_get_handlers();

/// Returns pointer to the first metric defintion
metric_t *metric_get_iterator_begin();

/// Returns pointer BEHIND the last metric defintion
metric_t *metric_get_iterator_end();

bool is_metric_enabled_for_handler(const metric_t *metric, const metric_handler_t *handler);

/// Enable metric for given handler
void metric_enable_for_handler(metric_t *metric, const metric_handler_t *handler);

/// Disable metric for given handler
void metric_disable_for_handler(metric_t *metric, const metric_handler_t *handler);

/// returns true, when metric min_interval_ms already passed and new record can be created
/// note: useful if obtaining record value takes significant time
bool metric_record_is_due(metric_t *metric);

#ifdef __cplusplus
}
#endif //__cplusplus
