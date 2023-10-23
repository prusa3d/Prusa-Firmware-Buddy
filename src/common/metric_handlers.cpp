#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "metric_handlers.h"
#include "stm32f4xx_hal.h"
#include "str_utils.hpp"
#include "timing.h"
#include "syslog.h"
#include "otp.hpp"
#include "sensor_data_buffer.h"
#include <option/development_items.h>
#include <config_store/store_instance.hpp>
#include <atomic>

#define TEXTPROTOCOL_POINT_MAXLEN 63
#define BUFFER_OLD_MS             1000 // after how many ms we flush the buffer

/// Used for indexing METRIC_VALUE_LOG entries. Should be called from a single thread, so we don't need to make this atomic.
static uint32_t metric_log_index_counter = 0;

static void textprotocol_append_point(StringBuilder &sb, metric_point_t *point, int timestamp_diff) {
    sb.append_string(point->metric->name);

    const auto append_string_escaped = [&](const char *str) {
        sb.append_char('"');

        while (char ch = *str++) {
            if (ch == '"') {
                sb.append_char('\\');
            }

            sb.append_char(ch);
        }

        sb.append_char('"');
    };

    int type = point->metric->type;

    if (point->error && point->metric->type != METRIC_VALUE_CUSTOM) {
        type = -1;
    }

    switch (type) {

    case METRIC_VALUE_CUSTOM:
        sb.append_string(point->value_stream);
        break;

    case METRIC_VALUE_FLOAT:
        sb.append_printf(" v=%f", double(point->value_float));
        break;

    case METRIC_VALUE_INTEGER:
        sb.append_printf(" v=%ii", point->value_int);
        break;

    case METRIC_VALUE_LOG:
        // Log - add _seq tag
        sb.append_printf(",_seq=%lu", metric_log_index_counter++);
        [[fallthrough]];

    case METRIC_VALUE_STRING: {
        sb.append_string(" v=");
        append_string_escaped(point->value_stream);
        break;
    }

    case METRIC_VALUE_EVENT:
        sb.append_string(" v=T");
        break;

        // Custom value for error
    case -1:
        sb.append_string(" error=");
        append_string_escaped(point->value_stream);
        break;

    default:
        sb.append_string(" error=\"Unknown value type\"");
        break;
    }

    sb.append_printf(" %i\n", timestamp_diff);
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

    ArrayStringBuilder<TEXTPROTOCOL_POINT_MAXLEN + 1> sb;
    textprotocol_append_point(sb, point, timestamp_diff);
    uart_send_line(sb.str());
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
        if (!buffer_header_size) {
            init_buffer();
        }

        int timestamp_diff = ticks_diff(point->timestamp, buffer_reference_timestamp);

        while (true) {
            StringBuilder builder(buffer, buffer_used);
            textprotocol_append_point(builder, point, timestamp_diff);

            // We've successfully appended the metric to the buffer -> finish
            if (builder.is_ok()) {
                buffer_used += builder.char_count();
                break;
            }

            // We've cropped over the buffer -> 'remove' the built message by replacing the first character with \0
            buffer[buffer_used] = '\0';

            // If there was anything else in the buffer than header, we send the buffer (make more space) and try again
            if (buffer_used != buffer_header_size) {
                send_buffer();
                continue;

            } else {
                // Otherwise we have to throw the metric out
                break;
            }
        }

        const bool buffer_almost_full = buffer_used + TEXTPROTOCOL_POINT_MAXLEN > sizeof(buffer);
        const bool buffer_becoming_old = ticks_diff(ticks_ms(), buffer_reference_timestamp) > BUFFER_OLD_MS;

        // send the buffer if it's (almost) full or old enough
        if (buffer_almost_full || buffer_becoming_old) {
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
        buffer_header_size = snprintf(
            buffer, sizeof(buffer),
            "<%i>1 - %s %s - - - msg=%i,tm=%lu,v=3 ",
            facility * 8 + severity, otp_get_mac_address_str().data(), appname, message_id++, buffer_reference_timestamp);

        buffer_used = buffer_header_size;
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
    char buffer[1024];
    size_t buffer_used = 0;
    size_t buffer_header_size = 0;
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
