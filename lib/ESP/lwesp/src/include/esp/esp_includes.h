/**
 * \file            esp_includes.h
 * \brief           All main includes
 */

/*
 * Copyright (c) 2018 Tilen Majerle
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
 */
#ifndef ESP_HDR_INCLUDES_H
#define ESP_HDR_INCLUDES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "esp_config.h"
#include "esp/esp_typedefs.h"
#include "esp/esp_buff.h"
#include "esp/esp_input.h"
#include "esp/esp_evt.h"
#include "esp/esp_debug.h"
#include "esp/esp_utilities.h"
#include "esp/esp_pbuf.h"
#include "esp/esp_conn.h"
#include "system/esp_sys.h"

#if ESP_CFG_MODE_STATION || __DOXYGEN__
#include "esp/esp_sta.h"
#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */
#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
#include "esp/esp_ap.h"
#endif /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if ESP_CFG_NETCONN || __DOXYGEN__
#include "esp/esp_netconn.h"
#endif /* ESP_CFG_NETCONN || __DOXYGEN__ */
#if ESP_CFG_PING || __DOXYGEN__
#include "esp/esp_ping.h"
#endif /* ESP_CFG_PING || __DOXYGEN__ */
#if ESP_CFG_WPS || __DOXYGEN__
#include "esp/esp_wps.h"
#endif /* ESP_CFG_WPS || __DOXYGEN__ */
#if ESP_CFG_SNTP || __DOXYGEN__
#include "esp/esp_sntp.h"
#endif /* ESP_CFG_SNTP || __DOXYGEN__ */
#if ESP_CFG_HOSTNAME || __DOXYGEN__
#include "esp/esp_hostname.h"
#endif /* ESP_CFG_HOSTNAME || __DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ESP_HDR_INCLUDES_H */
