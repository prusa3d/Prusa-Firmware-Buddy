/**
 * \file            esp_config.h
 * \brief           Configuration for ESP
 */

/*
 * Copyright (c) 2019 Tilen MAJERLE
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         $_version_$
 */
#ifndef ESP_HDR_CONFIG_H
#define ESP_HDR_CONFIG_H

/* User specific config which overwrites setup from esp_config_default.h file */

#if !__DOXYGEN__
#define ESP_CFG_NETCONN                     1
#define ESP_CFG_DBG                         ESP_DBG_ON
#define ESP_CFG_DBG_TYPES_ON                ESP_DBG_TYPE_TRACE | ESP_DBG_TYPE_STATE
#define ESP_CFG_DBG_IPD                     ESP_DBG_OFF
#define ESP_CFG_DBG_SERVER                  ESP_DBG_OFF
#define ESP_CFG_DBG_MQTT                    ESP_DBG_OFF
#define ESP_CFG_DBG_MEM                     ESP_DBG_OFF
#define ESP_CFG_DBG_PBUF                    ESP_DBG_OFF
#define ESP_CFG_DBG_CONN                    ESP_DBG_OFF
#define ESP_CFG_DBG_VAR                     ESP_DBG_OFF
#define ESP_CFG_RCV_BUFF_SIZE               0x1000

#define ESP_CFG_MEM_CUSTOM                  1

#define ESP_CFG_REST_CLIENT                 1

#define ESP_CFG_ESP32                       1
#define ESP_CFG_ESP8266                     1

#define ESP_CFG_IPD_MAX_BUFF_SIZE           1460
#define ESP_CFG_INPUT_USE_PROCESS           1
#define ESP_CFG_AT_ECHO                     0

#define ESP_CFG_USE_API_FUNC_EVT            1

#define ESP_CFG_MAX_CONNS                   5

#define ESP_CFG_DNS                         1
#define ESP_CFG_SNTP                        1
#define ESP_CFG_HOSTNAME                    1
#define ESP_CFG_WPS                         1
#define ESP_CFG_MDNS                        1
#define ESP_CFG_PING                        1

#define ESP_CFG_RESET_ON_INIT               1

#define ESP_CFG_CONN_MANUAL_TCP_RECEIVE     0

#if defined(WIN32)
#define ESP_CFG_SYS_PORT					ESP_SYS_PORT_WIN32
#endif

#endif /* !__DOXYGEN__ */

/* Include default configuration setup */
#include "esp/esp_config_default.h"
 
/**
 * \}
 */

#endif /* ESP_HDR_CONFIG_H */
