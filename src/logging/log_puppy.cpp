#include <logging/log.hpp>

#include <array>
#include <cstdio>
#include <logging/log_platform.hpp>

namespace logging {

void _log_event_valist(Severity severity, const Component *component, const char *fmt, va_list *args) {
    if (severity < component->lowest_severity) {
        return;
    }

    std::array<char, 128> message;
    vsnprintf(message.data(), message.size(), fmt, *args);
    FormattedEvent formatted_event {
        .timestamp = log_platform_timestamp_get(),
        .task_id = log_platform_task_id_get(),
        .component = component,
        .severity = severity,
        .message = message.data(),
    };
    log_task_process_event(&formatted_event);
}

} // namespace logging
