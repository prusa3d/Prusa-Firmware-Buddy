#include <stdbool.h>
#include "log_dest_shared.h"

#include "printf.h"

static const char *log_severity_to_str(log_severity_t severity) {
    switch (severity) {
    case LOG_SEVERITY_DEBUG:
        return "DEBUG";
    case LOG_SEVERITY_INFO:
        return "INFO ";
    case LOG_SEVERITY_WARNING:
        return "WARN ";
    case LOG_SEVERITY_ERROR:
        return "ERROR";
    case LOG_SEVERITY_CRITICAL:
        return "CRITI";
    default:
        return "?????";
    }
}

void log_format_simple(log_event_t *event, void (*out_fn)(char character, void *arg), void *arg) {
    // Format:
    // 3.32s [INFO - GUI:4] log message
    fctprintf(out_fn, arg, "%u.%03us [%s - %s:%i] ",
        event->timestamp.sec,
        event->timestamp.us / 1000,
        log_severity_to_str(event->severity),
        event->component->name,
        event->task_id);
    vfctprintf(out_fn, arg, event->fmt, *event->args);
}
