/**
 * \file            lwesp_opts_template.h
 * \brief           Template config file
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
// #ifndef LWESP_HDR_OPTS_H
// #define LWESP_HDR_OPTS_H

// /* Rename this file to "lwesp_opts.h" for your application */
// #include "dbg.h"

// /*
//  * Open "include/lwesp/lwesp_opt.h" and
//  * copy & replace here settings you want to change values
//  */
// #define LWESP_CFG_INPUT_USE_PROCESS 1
// #define LWESP_CFG_RESET_ON_INIT     1
// #define LWESP_CFG_RESTORE_ON_INIT   0
// #define LWESP_CFG_MAX_SSID_LENGTH   25
// #define LWESP_CFG_DBG               LWESP_DBG_ON
// #define LWESP_CFG_DBG_TYPES_ON      LWESP_DBG_TYPE_ALL
// #define LWESP_CFG_USE_API_FUNC_EVT 1
// #define LWESP_CFG_NETCONN          1
// #define LWESP_CFG_DBG_INIT         LWESP_DBG_ON
// #define LWESP_CFG_AT_ECHO          1

// #endif /* LWESP_HDR_OPTS_H */

/**
 * \file            esp_config_template.h
 * \brief           Template config file
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
#ifndef ESP_HDR_CONFIG_H
#define ESP_HDR_CONFIG_H

/* Rename this file to "esp_config.h" for your application */

#define ESP_CFG_INPUT_USE_PROCESS       1
#define ESP_CFG_RESET_ON_INIT           1
#define ESP_CFG_RESTORE_ON_INIT         1
#define ESP_CFG_MAX_SSID_LENGTH         32
#define ESP_CFG_NETCONN                 1
#define ESP_ALTCP                       1
#define ESP_CFG_CONN_POLL_INTERVAL      2000
#define ESP_CFG_CONN_MANUAL_TCP_RECEIVE 1
// This is a bit hacky, we had to adjust LwESP config processing to be able to
// set this one. Extended stack is necessary to start print from ESP event.
// It would be better to start print from different thread on signal.
#define ESP_SYS_THREAD_SS 512
#define ESP_CFG_HOSTNAME  1

#define ESP_CFG_DBG          ESP_DBG_OFF
#define ESP_CFG_DBG_MEM      ESP_DBG_ON
#define ESP_CFG_DBG_THREAD   ESP_DBG_ON
#define ESP_CFG_DBG_IPD      ESP_DBG_ON
#define ESP_CFG_DBG_SERVER   0
#define ESP_CFG_DBG_TYPES_ON (ESP_DBG_TYPE_STATE | ESP_DBG_TYPE_TRACE)

#include "dbg.h"
#define ESP_CFG_DBG_OUT(fmt, ...) _dbg(fmt, ##__VA_ARGS__)

#define ESP_CFG_MODE_ACCESS_POINT 0

/* After user configuration, call default config to merge config together */
#include "esp/esp_config_default.h"

/**
 * This configures LwESP, a library that talks to stock Espressif AT firmware
 * using UART providing TCP connection abstraction. This seems to work,
 * somehow, but there are some downsides.
 *
 * 1) There is only a custom flow controll based on sending TCP packet data
 * on explicit request. Still, in theory, the lack of HW flow controll can
 * cause receive DMA buffer overlow in case of combination of incomming
 * packets and commands from ESP.
 *
 * 2) We need to provide BSD sockets wrapper on top of LwESP provided Netconn
 * API. First, the current implementation is rather a draft than a seriously
 * implemented and tested piece of code. Second, the LwESP provided Netconn API
 * does not allow to implement all BSD sockets functionality. Moreover, some
 * functionality can only be provided at cost of both runtime and coding
 * overhead.
 *
 * 3) We run LwIP HTTP web server to handle both ESP(Wifi) and LwIP(Ethernet)
 * requests. We provide a LwESP wrapper implementing ALTCP API of the LwIP to
 * let the HTTP request from Wifi through. Yet, we cannot handle both Ethernet
 * and Wifi requests simulatenously using a single HTTP server instance. This
 * limits possible future simulataneous operation of both Ethernet and Wifi.
 *
 * 4) Even when it can be worked out, currently we keep separate network
 * buffers for complete LwIP network stack and the part of ESP network stack
 * that is not offloaded to ESP. As LwESP uses custom heap alocator and LwIP
 * uses mempoll allocator the memory regions are not easy to share. Such a
 * change would require chnages to the codebase of the libraries.
 *
 * 4) Last thing to mention is code overhead. Even when we use LwESP library,
 * still we are supposed to provide BSD socket and ALTCP API wrappers. This,
 * together with maintenance of 2019 LwESP codebase (we cannot use up to date
 * code due to flash limitation of our module) forces us to manage quite some
 * code that, of course, contains bugs. We might end up in a situation were
 * somefeatures will work only on Wifi while other will be working correcly on
 * Ethernet exclusively.
 *
 * There is a Plan B approach to ESP integration. It turned out quite easy to
 * implement a custom ESP firmware that just forwards received and accepts
 * packets to be sent by LwIP. This allows to implement simple alternative LwIP
 * network device. This way both Wifi and Ethernet are managed by the same
 * network stack and the same HTTP server (and the same client/server we would
 * possibly use in the future). Apart from the fact the network device
 * switching works out of the box this also solves flow control issue as the
 * packets not processed are not acknowledged. Also this reduces our code
 * complexity and simply drops both LwESP code and its network buffers. All
 * this comes at the cost of custom ESP firmware and keeping ESP processing
 * power and memory buffers unused (these are unused also when Ethernet is
 * operating).
 */

#endif /* ESP_HDR_CONFIG_H */
