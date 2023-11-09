#include "log.h"
#include "log_platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static log_destination_t *destinations_head = NULL;

extern log_component_t __start_log_components[]
#if __APPLE__
    __asm("section$start$__DATA$log_components")
#endif
        ;

#if __APPLE__
extern log_component_t __end_log_components[] __asm("section$end$__DATA$log_components");
#elif !defined(__arm__)
    #define __end_log_components __stop_log_components
extern log_component_t __end_log_components[];
#else
extern log_component_t __end_log_components[];
#endif

void log_destination_register(log_destination_t *destination) {
    destination->next = NULL;
    log_destination_t **pointer_next = &destinations_head;
    while (*pointer_next != NULL) {
        pointer_next = &((*pointer_next)->next);
    }
    *pointer_next = destination;
}

void log_destination_unregister(log_destination_t *destination) {
    for (log_destination_t **destination_pp = &destinations_head; *destination_pp != NULL;) {
        if (destination == *destination_pp) {
            *destination_pp = destination->next;
        } else {
            destination_pp = &((*destination_pp)->next);
        }
    }
}

void _log_event(log_severity_t severity, const log_component_t *component, const char *fmt, ...) {
    if (severity < component->lowest_severity) {
        return;
    }

    log_event_t event {};
    event.timestamp = log_platform_timestamp_get();
    event.task_id = log_platform_task_id_get();
    event.severity = severity;
    event.component = component;
    event.fmt = fmt;

    for (log_destination_t **destination_pp = &destinations_head; *destination_pp != NULL; destination_pp = &(*destination_pp)->next) {
        log_destination_t *destination = *destination_pp;
        if (severity >= destination->lowest_severity) {
            va_list args;
            va_start(args, fmt);
            event.args = &args;
            destination->log_event_fn(destination, &event);
            va_end(args);
        }
    }
}

log_component_t *log_component_find(const char *name) {
    for (log_component_t *component = __start_log_components; component < __end_log_components; component++) {
        if (strcmp(name, component->name) == 0) {
            return component;
        }
    }
    return NULL;
}

log_destination_t *log_destination_find(const char *name) {
    for (log_destination_t **destination_pp = &destinations_head; *destination_pp != NULL; destination_pp = &((*destination_pp)->next)) {
        log_destination_t *destination = *destination_pp;
        if (strcmp(destination->name, name) == 0) {
            return destination;
        }
    }
    return NULL;
}
