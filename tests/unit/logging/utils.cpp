#include "utils.hpp"
#include <logging/log.hpp>
#include <logging/log_task.hpp>

static std::optional<std::function<int()>> _log_platform_task_id_get = std::nullopt;
static std::optional<std::function<logging::Timestamp()>> _log_platform_timestamp_get = std::nullopt;

ScopeGuard with_log_platform_task_id_get(std::optional<std::function<int()>> func) {
    auto backup = _log_platform_task_id_get;
    _log_platform_task_id_get = func;
    return ScopeGuard([backup]() {
        _log_platform_task_id_get = backup;
    });
}

ScopeGuard with_log_platform_timestamp_get(std::optional<std::function<logging::Timestamp()>> func) {
    auto backup = _log_platform_timestamp_get;
    _log_platform_timestamp_get = func;
    return ScopeGuard([backup]() {
        _log_platform_timestamp_get = backup;
    });
}

namespace logging {

TaskId log_platform_task_id_get() {
    if (_log_platform_task_id_get.has_value()) {
        return _log_platform_task_id_get.value()();
    } else {
        return 0;
    }
}

Timestamp log_platform_timestamp_get() {
    if (_log_platform_timestamp_get.has_value()) {
        return _log_platform_timestamp_get.value()();
    } else {
        return { 0, 0 };
    }
}

} // namespace logging

static std::optional<std::function<void(logging::FormattedEvent *event)>> log_event_func = std::nullopt;

static void custom_log_event(logging::FormattedEvent *event) {
    if (log_event_func.has_value()) {
        log_event_func.value()(event);
    }
}

logging::Destination in_memory_log = {
    .lowest_severity = logging::Severity::debug,
    .log_event_fn = custom_log_event,
    .next = NULL,
};

ScopedInMemoryLog::ScopedInMemoryLog() {
    log_event_func = [&](logging::FormattedEvent *evt) {
        std::string message(evt->message);
        RecordedLog log { evt->timestamp, evt->severity, evt->task_id, evt->component, message };
        logs.push_back(log);
    };
    log_destination_register(&in_memory_log);
}

ScopedInMemoryLog::~ScopedInMemoryLog() {
    log_destination_unregister(&in_memory_log);
    log_event_func = std::nullopt;
}
