#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "metric_handlers.h"
#include "stm32f4xx_hal.h"
#include "sockets.h"

#define MAC_ADDR_START            0x1FFF781A //MM:MM:MM:SS:SS:SS
#define MAC_ADDR_SIZE             6
#define TEXTPROTOCOL_POINT_MAXLEN 63

static int textprotocol_append_point(char *buffer, int buffer_len, metric_point_t *point, int timestamp_diff) {
    if (point->error) {
        return snprintf(buffer, buffer_len,
            "[%i:%s:error:%s]", timestamp_diff, point->metric->name, point->error_msg);
    }

    switch (point->metric->type) {
    case METRIC_VALUE_FLOAT:
        return snprintf(buffer, buffer_len,
            "[%i:%s:f:%f]", timestamp_diff, point->metric->name, (double)point->value_float);
    case METRIC_VALUE_INTEGER:
        return snprintf(buffer, buffer_len,
            "[%i:%s:i:%i]", timestamp_diff, point->metric->name, point->value_int);
    case METRIC_VALUE_STRING:
        // TODO: escaping
        return snprintf(buffer, buffer_len,
            "[%i:%s:s:%s]", timestamp_diff, point->metric->name, point->value_str);
    case METRIC_VALUE_EVENT:
        return snprintf(buffer, buffer_len,
            "[%i:%s:e:]", timestamp_diff, point->metric->name);
    default:
        return snprintf(buffer, buffer_len,
            "[%i:%s:error:unknown value type]", timestamp_diff, point->metric->name);
    }
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

static char syslog_server_ipaddr[16] = { 0 };
static int syslog_server_port;

static int syslog_message_init(char *buffer, int buffer_len, uint32_t timestamp) {
    static int message_id = 0;
    const int facility = 1; // user level message
    const int severity = 6; // informational
    const char *appname = "buddy";

    // load mac addr and use it as hostname
    char hostname[18];
    {
        volatile uint8_t mac[MAC_ADDR_SIZE];
        for (uint8_t i = 0; i < MAC_ADDR_SIZE; i++)
            mac[i] = *((volatile uint8_t *)(MAC_ADDR_START + i));
        snprintf(hostname, sizeof(hostname),
            "%x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // What the.. format? Checkout RFC5425 (The Syslog Protocol)
    // https://tools.ietf.org/html/rfc5424
    return snprintf(
        buffer, buffer_len,
        "<%i>1 - %s %s - - - msg=%i,tm=%lu ",
        facility * 8 + severity, hostname, appname, message_id++, timestamp);
}

static void syslog_message_send(char *buffer, int buffer_len) {
    if (strlen(syslog_server_ipaddr) == 0)
        return;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return;

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(syslog_server_ipaddr);
    addr.sin_port = htons(syslog_server_port);

    if (sendto(sock, buffer, buffer_len, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }

    close(sock);
}

static void syslog_handler(metric_point_t *point) {
    static uint32_t buffer_oldest_timestamp = 0;
    static uint32_t buffer_newest_timestamp = 0;
    static char buffer_has_header = false;
    static char buffer[256];
    static int buffer_used = 0;

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
    bool buffer_becoming_old = (HAL_GetTick() - buffer_oldest_timestamp) > 1000;

    if (buffer_full || buffer_becoming_old) {
        syslog_message_send(buffer, buffer_used);
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
