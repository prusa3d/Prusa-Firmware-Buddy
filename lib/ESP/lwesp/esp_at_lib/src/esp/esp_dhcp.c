/**
 * \file            esp_dhcp.c
 * \brief           DHCP API
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
#include "esp/esp_dhcp.h"
#include "esp/esp_mem.h"

/**
 * \brief           Configure DHCP settings for station or access point (or both)
 *
 * Configuration changes will be saved in the NVS area of ESP device.
 *
 * \param[in]       sta: Set to `1` to affect station DHCP configuration, set to `0` to keep current setup
 * \param[in]       ap: Set to `1` to affect access point DHCP configuration, set to `0` to keep current setup
 * \param[in]       en: Set to `1` to enable DHCP, or `0` to disable (static IP)
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_dhcp_configure(uint8_t sta, uint8_t ap, uint8_t en,
                    const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWDHCP_SET;
    ESP_MSG_VAR_REF(msg).msg.wifi_cwdhcp.sta = sta;
    ESP_MSG_VAR_REF(msg).msg.wifi_cwdhcp.ap = ap;
    ESP_MSG_VAR_REF(msg).msg.wifi_cwdhcp.en = en;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}
