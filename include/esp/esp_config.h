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

#define ESP_CFG_INPUT_USE_PROCESS  1
#define ESP_CFG_RESET_ON_INIT      1
#define ESP_CFG_RESTORE_ON_INIT    0
#define ESP_CFG_MAX_SSID_LENGTH    25
#define ESP_CFG_NETCONN            1
#define ESP_ALTCP                  1
#define ESP_CFG_CONN_POLL_INTERVAL 2000
/* After user configuration, call default config to merge config together */
#include "esp/esp_config_default.h"

#endif /* ESP_HDR_CONFIG_H */
