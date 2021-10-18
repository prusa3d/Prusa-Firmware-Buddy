/**
 * \file            esp_ap.h
 * \brief           Access point API
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
#ifndef ESP_HDR_AP_H
#define ESP_HDR_AP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

/**
 * \ingroup         ESP
 * \defgroup        ESP_AP Access point
 * \brief           Access point
 * \{
 *
 * Functions to manage access point (AP) on ESP device.
 *
 * In order to be able to use AP feature, \ref ESP_CFG_MODE_ACCESS_POINT must be enabled.
 */

espr_t      esp_ap_getip(esp_ip_t* ip, esp_ip_t* gw, esp_ip_t* nm, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
espr_t      esp_ap_setip(const esp_ip_t* ip, const esp_ip_t* gw, const esp_ip_t* nm, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
espr_t      esp_ap_getmac(esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
espr_t      esp_ap_setmac(const esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

espr_t      esp_ap_configure(const char* ssid, const char* pwd, uint8_t ch, esp_ecn_t ecn, uint8_t max_sta, uint8_t hid, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

espr_t      esp_ap_list_sta(esp_sta_t* sta, size_t stal, size_t* staf, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif /* ESP_HDR_AP_H */
