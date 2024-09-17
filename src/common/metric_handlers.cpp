#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <logging/log.hpp>
#include "metric_handlers.h"
#include "stm32f4xx_hal.h"
#include "timing.h"
#include "syslog_transport.hpp"
#include "otp.hpp"
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

static int __attribute__((format(__printf__, 3, 4)))
append_format(char *buffer, int buffer_len, const char *format, ...) {
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
// SysLog Handler
//
// Note: This is not required to be in CCMRAM and can be moved to regular RAM if needed.
static __attribute__((section(".ccmram"))) SyslogTransport syslog_transport;

void metrics_reconfigure() {
    const auto host = config_store().metrics_host.get();

    // One symbol is not enough (for cases where people put in "-" or "x" or something there)
    if (strlen(host.data()) < 2) {
        config_store().enable_metrics.set(false);
    }

    if (config_store().enable_metrics.get()) {
        const auto port = config_store().metrics_port.get();
        syslog_transport.reopen(host.data(), port);

    } else {
        syslog_transport.reopen(nullptr, 0);
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

extern "C" void metric_handler(metric_point_t *point) {
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
        // Allow blocking - that allows the transport _not_ to copy the big
        // buffer and we run in our own thread.
        syslog_transport.send(buffer, buffer_used);
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
