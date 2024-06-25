#include <logging/log.hpp>

#include <logging/log_platform.hpp>
#include <logging/log_task.hpp>

namespace logging {

static Task __attribute__((section(".ccmram"))) log_task;

void _log_event_valist(Severity severity, const Component *component, const char *fmt, va_list *args) {
    if (severity < component->lowest_severity) {
        return;
    }

    logging::Event event {};
    event.timestamp = log_platform_timestamp_get();
    event.task_id = log_platform_task_id_get();
    event.severity = severity;
    event.component = component;
    event.fmt = fmt;
    event.args = args;
    log_task.send(&event);
}

} // namespace logging
