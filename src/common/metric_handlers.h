#pragma once
#include "metric.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

enum {
    METRIC_HANDLER_UART_ID,
    METRIC_HANDLER_SYSLOG_ID,
    METRIC_HANDLER_INFO_SCREEN,
};

extern metric_handler_t metric_handler_uart;
extern metric_handler_t metric_handler_syslog;
extern metric_handler_t metric_handler_info_screen;

void metric_handler_syslog_configure(const char *ip, int port);

#ifdef __cplusplus
}
#endif //__cplusplus
