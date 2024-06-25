#include <logging/log.hpp>

#include <logging/log_platform.hpp>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static logging::Destination *destinations_head = NULL;

extern logging::Component __start_log_components[]
#if __APPLE__
    __asm("section$start$__DATA$log_components")
#endif
        ;

#if __APPLE__
extern logging::Component __end_log_components[] __asm("section$end$__DATA$log_components");
#elif !defined(__arm__)
    #define __end_log_components __stop_log_components
extern logging::Component __end_log_components[];
#else
extern logging::Component __end_log_components[];
#endif

namespace logging {

void log_destination_register(Destination *destination) {
    destination->next = NULL;
    Destination **pointer_next = &destinations_head;
    while (*pointer_next != NULL) {
        pointer_next = &((*pointer_next)->next);
    }
    *pointer_next = destination;
}

void log_destination_unregister(Destination *destination) {
    for (Destination **destination_pp = &destinations_head; *destination_pp != NULL;) {
        if (destination == *destination_pp) {
            *destination_pp = destination->next;
        } else {
            destination_pp = &((*destination_pp)->next);
        }
    }
}

void _log_event(Severity severity, const Component *component, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _log_event_valist(severity, component, fmt, &args);
    va_end(args);
}

void log_task_process_event(FormattedEvent *event) {
    for (Destination **destination_pp = &destinations_head; *destination_pp != NULL; destination_pp = &(*destination_pp)->next) {
        Destination *destination = *destination_pp;
        if (event->severity >= destination->lowest_severity) {
            destination->log_event_fn(event);
        }
    }
}

Component *log_component_find(const char *name) {
    for (Component *component = __start_log_components; component < __end_log_components; component++) {
        if (strcmp(name, component->name) == 0) {
            return component;
        }
    }
    return NULL;
}

} // namespace logging
