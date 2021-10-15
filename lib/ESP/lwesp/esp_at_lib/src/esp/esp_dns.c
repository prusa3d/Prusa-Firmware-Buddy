/**
 * \file            esp_dns.c
 * \brief           DNS API
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
#include "esp/esp_private.h"
#include "esp/esp_dns.h"
#include "esp/esp_mem.h"

#if ESP_CFG_DNS || __DOXYGEN__

/**
 * \brief           Get IP address from host name
 * \param[in]       host: Pointer to host name to get IP for
 * \param[out]      ip: Pointer to \ref esp_ip_t variable to save IP
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_dns_gethostbyname(const char* host, esp_ip_t* const ip,
                        const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("host != NULL", host != NULL);
    ESP_ASSERT("ip != NULL", ip != NULL);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPDOMAIN;
    ESP_MSG_VAR_REF(msg).msg.dns_getbyhostname.host = host;
    ESP_MSG_VAR_REF(msg).msg.dns_getbyhostname.ip = ip;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 20000);
}

/**
 * \brief           Enable or disable custom DNS server configuration
 *
 * Configuration changes will be saved in the NVS area of ESP device.
 *
 * \param[in]       en: Set to `1` to enable, `0` to disable custom DNS configuration.
 *                  When disabled, default DNS servers are used as proposed by ESP AT commands firmware
 * \param[in]       s1: First server IP address in string format, set to `NULL` if not used
 * \param[in]       s2: Second server IP address in string format, set to `NULL` if not used.
 *                  Address `s1` cannot be the same as `s2`
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_dns_set_config(uint8_t en, const char* s1, const char* s2,
    const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPDNS_SET;
    ESP_MSG_VAR_REF(msg).msg.dns_setconfig.en = en;
    ESP_MSG_VAR_REF(msg).msg.dns_setconfig.s1 = s1;
    ESP_MSG_VAR_REF(msg).msg.dns_setconfig.s2 = s2;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

#endif /* ESP_CFG_DNS || __DOXYGEN__ */
