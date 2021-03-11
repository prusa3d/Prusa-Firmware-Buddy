/**
 * \file            lwesp_opts.h
 * \brief           ESP application options
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
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
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.0.0
 */
#ifndef LWESP_HDR_OPTS_H
#define LWESP_HDR_OPTS_H

/* Rename this file to "lwesp_opts.h" for your application */

/*
 * Open "include/lwesp/lwesp_opt.h" and
 * copy & replace here settings you want to change values
 */
#if !__DOXYGEN__
#define LWESP_CFG_NETCONN                     1
#define LWESP_CFG_NETCONN_RECEIVE_QUEUE_LEN   16

#define LWESP_CFG_DBG                         LWESP_DBG_ON
#define LWESP_CFG_DBG_TYPES_ON                LWESP_DBG_TYPE_TRACE | LWESP_DBG_TYPE_STATE
#define LWESP_CFG_DBG_IPD                     LWESP_DBG_OFF
#define LWESP_CFG_DBG_SERVER                  LWESP_DBG_OFF
#define LWESP_CFG_DBG_MQTT                    LWESP_DBG_OFF
#define LWESP_CFG_DBG_MEM                     LWESP_DBG_OFF
#define LWESP_CFG_DBG_PBUF                    LWESP_DBG_OFF
#define LWESP_CFG_DBG_CONN                    LWESP_DBG_OFF
#define LWESP_CFG_DBG_VAR                     LWESP_DBG_OFF
#define LWESP_CFG_RCV_BUFF_SIZE               0x1000

#define LWESP_CFG_MEM_CUSTOM                  1

#define LWESP_CFG_REST_CLIENT                 1

#define LWESP_CFG_ESP32                       1
#define LWESP_CFG_ESP8266                     1

#define LWESP_CFG_IPD_MAX_BUFF_SIZE           1460
#define LWESP_CFG_CONN_MAX_DATA_LEN           2048
#define LWESP_CFG_INPUT_USE_PROCESS           1
#define LWESP_CFG_AT_ECHO                     0

#define LWESP_CFG_USE_API_FUNC_EVT            1

#define LWESP_CFG_MAX_CONNS                   5

#define LWESP_CFG_DNS                         1
#define LWESP_CFG_SNTP                        1
#define LWESP_CFG_HOSTNAME                    1
#define LWESP_CFG_WPS                         1
#define LWESP_CFG_MDNS                        1
#define LWESP_CFG_PING                        1

#define LWESP_CFG_RESET_ON_INIT               1

#define LWESP_CFG_CONN_MANUAL_TCP_RECEIVE     1

#if defined(WIN32)
#define LWESP_CFG_SYS_PORT					LWESP_SYS_PORT_WIN32
#endif

#endif /* !__DOXYGEN__ */

#endif /* LWESP_HDR_OPTS_H */
