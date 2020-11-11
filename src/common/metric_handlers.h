#pragma once
#include "metric.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

enum {
    METRIC_HANDLER_UART_ID,
    METRIC_HANDLER_SYSLOG_ID,
};

extern metric_handler_t metric_handler_uart;
extern metric_handler_t metric_handler_syslog;

void metric_handler_syslog_configure(const char *ip, int port);

#ifdef __cplusplus
}
#endif //__cplusplus
