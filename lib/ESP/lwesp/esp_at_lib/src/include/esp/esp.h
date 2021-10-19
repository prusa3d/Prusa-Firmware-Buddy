/**
 * \file            esp.h
 * \brief           ESP AT commands parser
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
#ifndef ESP_HDR_H
#define ESP_HDR_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Get most important include files */
#include "esp/esp_includes.h"

/**
 * \defgroup        ESP ESP-AT Lib
 * \brief           ESP stack
 * \{
 */

espr_t      esp_init(esp_evt_fn cb_func, const uint32_t blocking);
espr_t      esp_reset(const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
espr_t      esp_reset_with_delay(uint32_t delay, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

espr_t      esp_restore(const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
espr_t      esp_set_at_baudrate(uint32_t baud, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
espr_t      esp_set_wifi_mode(esp_mode_t mode, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

espr_t      esp_set_server(uint8_t en, esp_port_t port, uint16_t max_conn, uint16_t timeout, esp_evt_fn cb, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

espr_t      esp_update_sw(const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

espr_t      esp_core_lock(void);
espr_t      esp_core_unlock(void);

espr_t      esp_device_set_present(uint8_t present, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
uint8_t     esp_device_is_present(void);

uint8_t     esp_device_is_esp8266(void);
uint8_t     esp_device_is_esp32(void);

uint8_t     esp_delay(const uint32_t ms);

uint8_t     esp_get_current_at_fw_version(esp_sw_version_t* const version);

/**
 * \brief           Set and format major, minor and patch values to firmware version
 * \param[in]       v: Version output, pointer to \ref esp_sw_version_t structure
 * \param[in]       major_: Major version
 * \param[in]       minor_: Minor version
 * \param[in]       patch_: Patch version
 * \hideinitializer
 */
#define     esp_set_fw_version(v, major_, minor_, patch_)          do { (v)->major = (major_); (v)->minor = (minor_); (v)->patch = (patch_); } while (0)

/**
 * \brief           Get minimal AT version supported by library
 * \param[out]      v: Version output, pointer to \ref esp_sw_version_t structure
 * \hideinitializer
 */
#define     esp_get_min_at_fw_version(v)   esp_set_fw_version(v, ESP_MIN_AT_VERSION_MAJOR_ESP8266, ESP_MIN_AT_VERSION_MINOR_ESP8266, ESP_MIN_AT_VERSION_PATCH_ESP8266)

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ESP_HDR_H */
