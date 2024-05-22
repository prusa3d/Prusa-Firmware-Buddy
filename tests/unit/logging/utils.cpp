#include "utils.hpp"
#include "log.h"

static std::optional<std::function<int()>> _log_platform_task_id_get = std::nullopt;
static std::optional<std::function<log_timestamp_t()>> _log_platform_timestamp_get = std::nullopt;

ScopeGuard with_log_platform_task_id_get(std::optional<std::function<int()>> func) {
    auto backup = _log_platform_task_id_get;
    _log_platform_task_id_get = func;
    return ScopeGuard([backup]() {
        _log_platform_task_id_get = backup;
    });
}

ScopeGuard with_log_platform_timestamp_get(std::optional<std::function<log_timestamp_t()>> func) {
    auto backup = _log_platform_timestamp_get;
    _log_platform_timestamp_get = func;
    return ScopeGuard([backup]() {
        _log_platform_timestamp_get = backup;
    });
}

int log_platform_task_id_get() {
    if (_log_platform_task_id_get.has_value()) {
        return _log_platform_task_id_get.value()();
    } else {
        return 0;
    }
}

log_timestamp_t log_platform_timestamp_get() {
    if (_log_platform_timestamp_get.has_value()) {
        return _log_platform_timestamp_get.value()();
    } else {
        return { 0, 0 };
    }
}

static std::optional<std::function<void(log_destination_t *destination, log_event_t *event)>> log_event_func = std::nullopt;

static void custom_log_event(log_destination_t *destination, log_event_t *event) {
    if (log_event_func.has_value()) {
        log_event_func.value()(destination, event);
    }
}

log_destination_t in_memory_log = {
    .name = "in-memory",
    .lowest_severity = LOG_SEVERITY_DEBUG,
    .log_event_fn = custom_log_event,
    .log_format_fn = NULL,
    .next = NULL,
};

ScopedInMemoryLog::ScopedInMemoryLog() {
    log_event_func = [&](log_destination_t *destination, log_event_t *evt) {
        char message_buffer[1024];
        vsnprintf(message_buffer, sizeof(message_buffer), evt->fmt, *evt->args);
        std::string message(message_buffer);
        RecordedLog log { evt->timestamp, evt->severity, evt->task_id, evt->component, message };
        logs.push_back(log);
    };
    log_destination_register(&in_memory_log);
}

ScopedInMemoryLog::~ScopedInMemoryLog() {
    log_destination_unregister(&in_memory_log);
    log_event_func = std::nullopt;
}
