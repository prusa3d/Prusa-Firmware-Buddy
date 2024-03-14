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

/**
 * @brief Initialize metric handlers.
 * Used by the syslog hanlder to pull config from eeprom and init mutex.
 */
void metric_handlers_init();

/**
 * @brief Configure syslog handler address and port.
 * @param ip IP address or short hostname
 * @param port port number
 */
void metric_handler_syslog_configure(const char *ip, uint16_t port);

/**
 * @brief Get address of the metrics syslog host.
 * @return IP address or short hostname
 */
const char *metric_handler_syslog_get_host();

/**
 * @brief Get port of the metrics syslog host.
 * @return port number
 */
uint16_t metric_handler_syslog_get_port();

#ifdef __cplusplus
}
#endif //__cplusplus
