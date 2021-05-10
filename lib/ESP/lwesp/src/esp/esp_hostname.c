/**
 * \file            esp_hostname.c
 * \brief           Hostname API
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
#include "esp/esp_private.h"
#include "esp/esp_hostname.h"
#include "esp/esp_mem.h"

#if ESP_CFG_HOSTNAME || __DOXYGEN__

/**
 * \brief           Set hostname of WiFi station
 * \param[in]       hostname: Name of ESP host
 * \param[in]       evt_fn: Callback function called when command is finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_hostname_set(const char* hostname,
                    const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("hostname != NULL", hostname != NULL);   /* Assert input parameters */

    ESP_MSG_VAR_ALLOC(msg);
    ESP_MSG_VAR_SET_EVT(msg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWHOSTNAME_SET;
    ESP_MSG_VAR_REF(msg).msg.wifi_hostname.hostname_set = hostname;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Get hostname of WiFi station
 * \param[in]       hostname: Pointer to output variable holding memory to save hostname
 * \param[in]       length: Length of buffer for hostname. Length includes memory for `NULL` termination
 * \param[in]       evt_fn: Callback function called when command is finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_hostname_get(char* hostname, size_t length,
                    const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("hostname != NULL", hostname != NULL);   /* Assert input parameters */
    ESP_ASSERT("length > 0", length > 0);       /* Assert input parameters */

    ESP_MSG_VAR_ALLOC(msg);
    ESP_MSG_VAR_SET_EVT(msg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWHOSTNAME_GET;
    ESP_MSG_VAR_REF(msg).msg.wifi_hostname.hostname_get = hostname;
    ESP_MSG_VAR_REF(msg).msg.wifi_hostname.length = length;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

#endif /* ESP_CFG_HOSTNAME || __DOXYGEN__ */
