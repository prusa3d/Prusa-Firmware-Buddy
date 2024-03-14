#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "metric_handlers.h"
#include "stm32f4xx_hal.h"
#include "timing.h"
#include "syslog.h"
#include "otp.hpp"
#include "sensor_data_buffer.h"
#include <option/development_items.h>
#include <config_store/store_instance.hpp>
#include <atomic>
#include <algorithm>
#include <inttypes.h>

#define TEXTPROTOCOL_POINT_MAXLEN 63
#define BUFFER_OLD_MS             1000 // after how many ms we flush the buffer

static int textprotocol_append_escaped(char *buffer, int buffer_len, char *val) {
    int appended = 0;
    while (*val != 0 && buffer_len > 0) {
        char ch = *(val++);
        if (ch == '"' || ch == '\\') {
            if (buffer_len < 2) {
                break;
            }
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

static int append_format(char *buffer, int buffer_len, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, std::max(buffer_len, 0), format, args);
    assert(length >= 0 && "unexpected snprintf encoding error");
    va_end(args);
    return length;
}

static int textprotocol_append_point(char *buffer, int buffer_len, metric_point_t *point, int timestamp_diff) {
    int buffer_used;
    if (point->metric->type == METRIC_VALUE_CUSTOM && !point->error) {
        buffer_used = append_format(buffer, buffer_len, "%s%s", point->metric->name, point->value_custom);
    } else {
        buffer_used = append_format(buffer, buffer_len, "%s ", point->metric->name);
    }

    if (point->metric->type == METRIC_VALUE_CUSTOM && !point->error) {
    } else if (point->error) {
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "error=\"");
        buffer_used += textprotocol_append_escaped(buffer + buffer_used, buffer_len - buffer_used, point->error_msg);
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "\"");
    } else if (point->metric->type == METRIC_VALUE_FLOAT) {
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "v=%f", (double)point->value_float);
    } else if (point->metric->type == METRIC_VALUE_INTEGER) {
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "v=%ii", point->value_int);
    } else if (point->metric->type == METRIC_VALUE_STRING) {
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "v=\"");
        buffer_used += textprotocol_append_escaped(buffer + buffer_used, buffer_len - buffer_used, point->value_str);
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "\"");
    } else if (point->metric->type == METRIC_VALUE_EVENT) {
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "v=T");
    } else {
        buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, "error=\"Unknown value type\"");
    }

    buffer_used += append_format(buffer + buffer_used, buffer_len - buffer_used, " %i\n", timestamp_diff);
    return buffer_used;
}

//
// UART Handler
//

// TODO: encapsulate huart6 access in hwio.h (and get rid of externs!)
// extern UART_HandleTypeDef huart6;

static void uart_send_line([[maybe_unused]] const char *line) {
    // TODO: Use DMA
    // @@TODO solve usart clash with MMU
    //    HAL_UART_Transmit(&huart6, (uint8_t *)line, strlen(line), HAL_MAX_DELAY);
    //    HAL_UART_Transmit(&huart6, (uint8_t *)"\r\n", 2, HAL_MAX_DELAY);
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
osMutexDef(syslog_server_lock);
osMutexId syslog_server_lock_id;
static char syslog_server_ipaddr[config_store_ns::metrics_host_size + 1] = "";
static uint16_t syslog_server_port = 0;
static syslog_transport_t syslog_transport;
static std::atomic<bool> reinit = false; ///< Close and reopen connection

void metric_handlers_init() {
    // Mutex for syslog server address and port
    syslog_server_lock_id = osMutexCreate(osMutex(syslog_server_lock));

    // Init syslog handler address and port from eeprom
    const MetricsAllow metrics_allow = config_store().metrics_allow.get();
    if ((metrics_allow == MetricsAllow::One || metrics_allow == MetricsAllow::All)
        && config_store().metrics_init.get()) {
        const char *host = config_store().metrics_host.get_c_str();
        const uint16_t port = config_store().metrics_port.get();
        metric_handler_syslog_configure(host, port);
    }
}

static int syslog_message_init(char *buffer, int buffer_len, int64_t timestamp) {
    static int message_id = 0;
    const int facility = 1; // user level message
    const int severity = 6; // informational
    const char *appname = "buddy";

    // What the.. format? Checkout RFC5425 (The Syslog Protocol)
    // https://tools.ietf.org/html/rfc5424
    return snprintf(
        buffer, buffer_len,
        "<%i>1 - %s %s - - - msg=%i,tm=%" PRId64 ",v=4 ",
        facility * 8 + severity, otp_get_mac_address_str().data(), appname, message_id++, timestamp);
}

static void syslog_handler(metric_point_t *point) {
    static uint32_t buffer_reference_timestamp = 0;
    static char buffer_has_header = false;
    static char buffer[1024];
    static unsigned int buffer_used = 0;

    auto init_header = [&]() {
        int64_t absolute_timestamp_us = get_timestamp_us();
        buffer_reference_timestamp = static_cast<uint32_t>(absolute_timestamp_us);
        buffer_used = syslog_message_init(buffer, sizeof(buffer), absolute_timestamp_us);
        buffer_has_header = true;
    };

    auto append_point = [&]() {
        int timestamp_diff = ticks_diff(point->timestamp, buffer_reference_timestamp);
        int buffer_used_by_point = textprotocol_append_point(
            buffer + buffer_used, sizeof(buffer) - buffer_used, point, timestamp_diff);
        if (buffer_used + buffer_used_by_point < sizeof(buffer)) {
            buffer_used += buffer_used_by_point;
            return true;
        } else {
            buffer[buffer_used] = '\0'; // drop the incomplete point from the buffer
            return false;
        }
    };

    if (!buffer_has_header) {
        init_header();
    }
    bool point_fit = append_point();
    bool buffer_full = (buffer_used + TEXTPROTOCOL_POINT_MAXLEN > sizeof(buffer)) || !point_fit;
    bool buffer_becoming_old = ticks_diff(ticks_us(), buffer_reference_timestamp) > (BUFFER_OLD_MS * 1000);

    // send the buffer if it's full or old enough
    if (buffer_full || buffer_becoming_old) {
        // is the socket ready?
        bool open = syslog_transport_check_is_open(&syslog_transport);

        // Request for reinit
        if (reinit.exchange(false) && open) {
            syslog_transport_close(&syslog_transport); // Close to use new host and port
            open = false;
        }

        // if not, try to open the socket
        if (!open) {
            if (osMutexWait(syslog_server_lock_id, osWaitForever) != osOK) {
                return; // Could not get mutex, maybe OS was killed
            }
            open = syslog_transport_open(&syslog_transport, syslog_server_ipaddr, syslog_server_port);
            osMutexRelease(syslog_server_lock_id);
        }

        // try to send the message if we have an open socket
        if (open) {
            bool sent = syslog_transport_send(&syslog_transport, buffer, buffer_used);
            if (!sent) {
                syslog_transport_close(&syslog_transport);
            }
        }

        buffer_used = 0;
        buffer_has_header = false;
    }

    if (!point_fit) {
        init_header();
        if (!append_point()) {
            assert(false && "point should always fit in a new buffer");
        }
    }
}

void metric_handler_syslog_configure(const char *ip, uint16_t port) {
    if (osMutexWait(syslog_server_lock_id, osWaitForever) != osOK) {
        return; // Could not get mutex, maybe OS was killed
    }
    strlcpy(syslog_server_ipaddr, ip, sizeof(syslog_server_ipaddr));
    syslog_server_port = port;
    reinit = true; // Close and reopen connection
    osMutexRelease(syslog_server_lock_id);
}

const char *metric_handler_syslog_get_host() {
    return syslog_server_ipaddr;
}

uint16_t metric_handler_syslog_get_port() {
    return syslog_server_port;
}

metric_handler_t metric_handler_syslog = {
    .identifier = METRIC_HANDLER_SYSLOG_ID,
    .name = "SYSLOG",
    .on_metric_registered_fn = NULL,
    .handle_fn = syslog_handler,
};

metric_handler_t metric_handler_info_screen = {
    .identifier = METRIC_HANDLER_INFO_SCREEN,
    .name = "SENSOR_INFO_SCREEN",
    .on_metric_registered_fn = NULL,
    .handle_fn = info_screen_handler,
};
