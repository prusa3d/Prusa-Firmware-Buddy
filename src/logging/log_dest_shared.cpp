#include <logging/log_dest_shared.hpp>

#include <printf/printf.h>

namespace logging {

static const char *log_severity_to_str(Severity severity) {
    switch (severity) {
    case Severity::debug:
        return "DEBUG";
    case Severity::info:
        return "INFO ";
    case Severity::warning:
        return "WARN ";
    case Severity::error:
        return "ERROR";
    case Severity::critical:
        return "CRITI";
    default:
        return "?????";
    }
}

void log_format_simple(FormattedEvent *event, void (*out_fn)(char character, void *arg), void *arg) {
    // Format:
    // 3.32s [INFO - GUI:4] log message
    fctprintf(out_fn, arg, "%u.%03us [%s - %s:%i] %s",
        event->timestamp.sec,
        event->timestamp.us / 1000,
        log_severity_to_str(event->severity),
        event->component->name,
        event->task_id,
        event->message);
}

void log_format_honeybee_node(FormattedEvent *event, void (*out_fn)(char character, void *arg), void *arg) {
    // Format:
    // [GUI:4] log message
    // we don't include severity & time as those are part of the diagnostic message already
    fctprintf(out_fn, arg, "[%s:%i] %s",
        event->component->name,
        event->task_id,
        event->message);
}

} // namespace logging
