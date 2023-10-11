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

/// Used for indexing METRIC_VALUE_LOG entries. Should be called from a single thread, so we don't need to make this atomic.
static uint32_t metric_log_index_counter = 0;

static int textprotocol_append_point(char *buffer, int buffer_len, metric_point_t *point, int timestamp_diff) {
// If we've clipped already, we don't need to continue further with snprintf
// Same logic applies for the same checks further in this function
#define CHECK_BUFFER_END           \
    if (buffer_used >= buffer_len) \
    return buffer_used

    int buffer_used = snprintf(buffer, buffer_len, "%s", point->metric->name);
    CHECK_BUFFER_END;

    if (point->metric->type == METRIC_VALUE_CUSTOM) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "%s", point->value_str_log_custom);
    } else if (point->metric->type == METRIC_VALUE_LOG) {
        // Log -> we need to add a tag with an unique value each time to prevent the values from being overriden
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, ",_seq=%lu ", metric_log_index_counter++);
    } else {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, " ");
    }
    CHECK_BUFFER_END;

    if (point->metric->type == METRIC_VALUE_CUSTOM) {
    } else if (point->error) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "error=\"");
        CHECK_BUFFER_END;

        buffer_used += textprotocol_append_escaped(buffer + buffer_used, buffer_len - buffer_used, point->error_msg);
        CHECK_BUFFER_END;

        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "\"");
    } else if (point->metric->type == METRIC_VALUE_FLOAT) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=%f", (double)point->value_float);
    } else if (point->metric->type == METRIC_VALUE_INTEGER) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=%ii", point->value_int);
    } else if (point->metric->type == METRIC_VALUE_STRING || point->metric->type == METRIC_VALUE_LOG) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=\"");
        CHECK_BUFFER_END;

        buffer_used += textprotocol_append_escaped(buffer + buffer_used, buffer_len - buffer_used, point->value_str_log_custom);
        CHECK_BUFFER_END;

        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "\"");
    } else if (point->metric->type == METRIC_VALUE_EVENT) {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "v=T");
    } else {
        buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, "error=\"Unknown value type\"");
    }
    CHECK_BUFFER_END;

    buffer_used += snprintf(buffer + buffer_used, buffer_len - buffer_used, " %i\n", timestamp_diff);
    return buffer_used;

#undef CHECK_BUFFER_END
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

namespace {

class MetricsBuffer {
public:
    void append(metric_point_t *point) {
        if (!buffer_has_header) {
            init_buffer();
        }

        int timestamp_diff = ticks_diff(point->timestamp, buffer_reference_timestamp);

        size_t buffer_used_for_metric = textprotocol_append_point(
            buffer + buffer_used, sizeof(buffer) - buffer_used, point, timestamp_diff);

        if (buffer_used_for_metric >= sizeof(buffer) - buffer_used) {
            // last metric didn't fit, send the buffer without it
            buffer[buffer_used] = '\0';
            send_buffer();

            // add the metric again to a fresh buffer
            buffer_used_for_metric = textprotocol_append_point(
                buffer + buffer_used, sizeof(buffer) - buffer_used, point, timestamp_diff);

            if (buffer_used_for_metric >= sizeof(buffer) - buffer_used) {
                // doesn't even fit again, discard
                buffer[buffer_used] = '\0';
                return;
            }
        }

        buffer_used += buffer_used_for_metric;

        bool buffer_full = buffer_used + TEXTPROTOCOL_POINT_MAXLEN > sizeof(buffer);
        bool buffer_becoming_old = ticks_diff(ticks_ms(), buffer_reference_timestamp) > BUFFER_OLD_MS;

        // send the buffer if it's (almost) full or old enough
        if (buffer_full || buffer_becoming_old) {
            send_buffer();
        }
    }

private:
    void init_buffer() {
        const int facility = 1; // user level message
        const int severity = 6; // informational
        const char *appname = "buddy";

        buffer_reference_timestamp = ticks_ms();

        // What the.. format? Checkout RFC5425 (The Syslog Protocol)
        // https://tools.ietf.org/html/rfc5424
        buffer_used = snprintf(
            buffer, sizeof(buffer),
            "<%i>1 - %s %s - - - msg=%i,tm=%lu,v=3 ",
            facility * 8 + severity, otp_get_mac_address_str().data(), appname, message_id++, buffer_reference_timestamp);

        buffer_has_header = true;
    }

    void send_buffer() {
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
            if (!sent)
                syslog_transport_close(&syslog_transport);
        }

        init_buffer();
    }

    int message_id { 0 };
    uint32_t buffer_reference_timestamp { 0 };
    bool buffer_has_header { false };
    char buffer[1024];
    size_t buffer_used { 0 };
};

} // namespace

static MetricsBuffer metrics_buffer;

static void syslog_handler(metric_point_t *point) {
    metrics_buffer.append(point);
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
