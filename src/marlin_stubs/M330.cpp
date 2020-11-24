#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include "M330.h"
#include "metric.h"
#include "metric_handlers.h"
#include <stdint.h>

static metric_handler_t *selected_handler = NULL;

void PrusaGcodeSuite::M330() {
    bool handler_found = false;
    for (metric_handler_t **handlers = metric_get_handlers(); *handlers != NULL; handlers++) {
        metric_handler_t *handler = *handlers;
        if (strcmp(handler->name, parser.string_arg) == 0) {
            selected_handler = handler;
            handler_found = true;
        }
    }
    if (handler_found) {
        SERIAL_ECHO_MSG("handler set");
    } else {
        SERIAL_ECHO_MSG("handler not found");
    }
}

void PrusaGcodeSuite::M331() {
    if (selected_handler == NULL) {
        SERIAL_ECHO_MSG("handler not set");
        return;
    }

    metric_t *metric = metric_get_linked_list();
    while (metric) {
        if (strcmp(metric->name, parser.string_arg) == 0) {
            metric->enabled_handlers |= (1 << selected_handler->identifier);
            SERIAL_ECHO_MSG("metric enabled");
            return;
        }
        metric = metric->next;
    }

    SERIAL_ECHO_MSG("metric not found");
}

void PrusaGcodeSuite::M332() {
    if (selected_handler == NULL) {
        SERIAL_ECHO_MSG("handler not set");
        return;
    }

    metric_t *metric = metric_get_linked_list();
    while (metric) {
        if (strcmp(metric->name, parser.string_arg) == 0) {
            metric->enabled_handlers &= ~(1 << selected_handler->identifier);
            SERIAL_ECHO_MSG("metric disabled");
            return;
        }
        metric = metric->next;
    }

    SERIAL_ECHO_MSG("metric not found");
}

void PrusaGcodeSuite::M333() {
    if (selected_handler == NULL) {
        SERIAL_ECHO_MSG("handler not set");
        return;
    }

    metric_t *metric = metric_get_linked_list();
    while (metric) {
        bool is_enabled = metric->enabled_handlers & (1 << selected_handler->identifier);
        SERIAL_ECHO(is_enabled ? '1' : '0');
        SERIAL_ECHO(" ");
        SERIAL_ECHOLN(metric->name);
        metric = metric->next;
    }
}

void PrusaGcodeSuite::M334() {
    if (selected_handler == NULL) {
        SERIAL_ECHO_MSG("handler not set");
        return;
    }

    if (selected_handler->identifier == METRIC_HANDLER_SYSLOG_ID) {
        char ipaddr[16];
        int port;
        int read = sscanf(parser.string_arg, "%16s %i", ipaddr, &port);
        if (read == 2) {
            metric_handler_syslog_configure(ipaddr, port);
            SERIAL_ECHOLN("syslog handler configured");
        } else {
            metric_handler_syslog_configure("", 0);
            SERIAL_ECHOLN("does not match '<address> <port>' pattern; disabling syslog handler");
        }
    } else {
        SERIAL_ECHOLN("selected handler does not support configuration");
    }
}
