#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "metric_handlers.h"
#include "stm32f4xx_hal.h"
#include "timing.h"
#include "syslog.h"
#include "otp.h"

#define TEXTPROTOCOL_POINT_MAXLEN 63
#define BUFFER_OLD_MS             1000 // after how many ms we flush the buffer

static int textprotocol_append_escaped(char *buffer, int buffer_len, char *val) {
    int appended = 0;
    while (*val != 0 && buffer_len > 0) {
        char ch = *(val++);
        if (ch == '"') {
            if (buffer_len < 2)
                break;
            buffer[0] = '\\';
            buffer[1] = ch;
            appended += 2;
            buffer += 2;
            buffer_len -= 2;
        } else {
            buffer[0] = ch;
            buffer_len -= 1;
            buffer += 1;
            appended += 1;
        }
    }
    return appended;
}

static int textprotocol_append_point(char *buffer, int buffer_len, metric_point_t *point, int timestamp_diff) {
    int buffer_used;
    if (point->metric->type == METRIC_VALUE_CUSTOM) {
        buffer_used = snprintf(buffer, buffer_len, "%s%s", point->metric->name, point->value_custom);
    } else {
        buffer_used = snprintf(buffer, buffer_len, "%s ", point->metric->name);
    }

    if (point->metric->type == METRIC_VALUE_CUSTOM) {
    } else if (point->error) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "error=\"");
        buffer_used += textprotocol_append_escaped(buffer + buffer_used, buffer_len - buffer_used, point->error_msg);
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "\"");
    } else if (point->metric->type == METRIC_VALUE_FLOAT) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=%f", (double)point->value_float);
    } else if (point->metric->type == METRIC_VALUE_INTEGER) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=%ii", point->value_int);
    } else if (point->metric->type == METRIC_VALUE_STRING) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=\"");
        buffer_used += textprotocol_append_escaped(buffer + buffer_used, buffer_len - buffer_used, point->value_str);
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "\"");
    } else if (point->metric->type == METRIC_VALUE_EVENT) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=T");
    } else {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "error=\"Unknown value type\"");
    }

    buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, " %i\n", timestamp_diff);
    return buffer_used;
}

//
// UART Handler
//

// TODO: encapsulate huart6 access in hwio.h (and get rid of externs!)
extern UART_HandleTypeDef huart6;

static void uart_send_line(const char *line) {
    // TODO: Use DMA
    HAL_UART_Transmit(&huart6, (uint8_t *)line, strlen(line), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart6, (uint8_t *)"\r\n", 2, HAL_MAX_DELAY);
}

static void uart_handler(metric_point_t *point) {
    static int last_reported_timestamp = 0;
    int timestamp_diff = point->timestamp - last_reported_timestamp;
    last_reported_timestamp = point->timestamp;

    char line[TEXTPROTOCOL_POINT_MAXLEN + 1];
    textprotocol_append_point(line, sizeof(line), point, timestamp_diff);
    uart_send_line(line);
}

metric_handler_t metric_handler_uart = {
    .identifier = METRIC_HANDLER_UART_ID,
    .name = "UART",
    .on_metric_registered_fn = NULL,
    .handle_fn = uart_handler,
};

//
// SysLog Handler
//

static char syslog_server_ipaddr[16] = "";
static int syslog_server_port = 8500;
static syslog_transport_t syslog_transport;

static int syslog_message_init(char *buffer, int buffer_len, uint32_t timestamp) {
    static int message_id = 0;
    const int facility = 1; // user level message
    const int severity = 6; // informational
    const char *appname = "buddy";

    // What the.. format? Checkout RFC5425 (The Syslog Protocol)
    // https://tools.ietf.org/html/rfc5424
    return snprintf(
        buffer, buffer_len,
        "<%i>1 - %s %s - - - msg=%i,tm=%lu,v=2 ",
        facility * 8 + severity, otp_get_mac_address_str(), appname, message_id++, timestamp);
}

static void syslog_handler(metric_point_t *point) {
    static uint32_t buffer_oldest_timestamp = 0;
    static uint32_t buffer_newest_timestamp = 0;
    static char buffer_has_header = false;
    static char buffer[1024] __attribute__((section(".ccmram")));
    static unsigned int buffer_used = 0;

    if (!buffer_has_header) {
        buffer_oldest_timestamp = point->timestamp;
        buffer_newest_timestamp = point->timestamp;
        buffer_used = syslog_message_init(buffer, sizeof(buffer), buffer_oldest_timestamp);
        buffer_has_header = true;
    }

    int timestamp_diff = point->timestamp - buffer_newest_timestamp;
    buffer_used += textprotocol_append_point(
        buffer + buffer_used, sizeof(buffer) - buffer_used, point, timestamp_diff);
    buffer_newest_timestamp = point->timestamp;

    bool buffer_full = buffer_used + TEXTPROTOCOL_POINT_MAXLEN > sizeof(buffer);
    bool buffer_becoming_old = ticks_diff(ticks_ms(), buffer_oldest_timestamp) > BUFFER_OLD_MS;

    // send the buffer if it's full or old enough
    if (buffer_full || buffer_becoming_old) {
        // is the socket ready?
        bool open = syslog_transport_check_is_open(&syslog_transport);
        // if not, try to open the socket
        if (!open)
            open = syslog_transport_open(&syslog_transport, syslog_server_ipaddr, syslog_server_port);
        // try to send the message if we have an open socket
        if (open) {
            bool sent = syslog_transport_send(&syslog_transport, buffer, buffer_used);
            if (!sent)
                syslog_transport_close(&syslog_transport);
        }

        buffer_used = 0;
        buffer_has_header = false;
    }
}

void metric_handler_syslog_configure(const char *ip, int port) {
    strlcpy(syslog_server_ipaddr, ip, sizeof(syslog_server_ipaddr));
    syslog_server_port = port;
}

metric_handler_t metric_handler_syslog = {
    .identifier = METRIC_HANDLER_SYSLOG_ID,
    .name = "SYSLOG",
    .on_metric_registered_fn = NULL,
    .handle_fn = syslog_handler,
};
