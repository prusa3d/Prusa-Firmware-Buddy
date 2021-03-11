/**
 * \file            lwesp.h
 * \brief           LwESP
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
#ifndef LWESP_HDR_H
#define LWESP_HDR_H

#include "lwesp/lwesp_includes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        LWESP Lightweight ESP-AT parser library
 * \brief           Lightweight ESP-AT parser
 * \{
 */

lwespr_t    lwesp_init(lwesp_evt_fn cb_func, const uint32_t blocking);
lwespr_t    lwesp_reset(const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
lwespr_t    lwesp_reset_with_delay(uint32_t delay, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

lwespr_t    lwesp_restore(const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
lwespr_t    lwesp_set_at_baudrate(uint32_t baud, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
lwespr_t    lwesp_set_wifi_mode(lwesp_mode_t mode, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
lwespr_t    lwesp_get_wifi_mode(lwesp_mode_t* mode, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

lwespr_t    lwesp_set_server(uint8_t en, lwesp_port_t port, uint16_t max_conn, uint16_t timeout, lwesp_evt_fn cb, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

lwespr_t    lwesp_update_sw(const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

lwespr_t    lwesp_core_lock(void);
lwespr_t    lwesp_core_unlock(void);

lwespr_t    lwesp_device_set_present(uint8_t present, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
uint8_t     lwesp_device_is_present(void);

uint8_t     lwesp_device_is_esp8266(void);
uint8_t     lwesp_device_is_esp32(void);

uint8_t     lwesp_delay(const uint32_t ms);

uint8_t     lwesp_get_current_at_fw_version(lwesp_sw_version_t* const version);

/**
 * \brief           Set and format major, minor and patch values to firmware version
 * \param[in]       v: Version output, pointer to \ref lwesp_sw_version_t structure
 * \param[in]       major_: Major version
 * \param[in]       minor_: Minor version
 * \param[in]       patch_: Patch version
 * \hideinitializer
 */
#define     lwesp_set_fw_version(v, major_, minor_, patch_)          do { (v)->major = (major_); (v)->minor = (minor_); (v)->patch = (patch_); } while (0)

/**
 * \brief           Get minimal AT version supported by library
 * \param[out]      v: Version output, pointer to \ref lwesp_sw_version_t structure
 * \hideinitializer
 */
#define     lwesp_get_min_at_fw_version(v)   lwesp_set_fw_version(v, LWESP_MIN_AT_VERSION_MAJOR_ESP8266, LWESP_MIN_AT_VERSION_MINOR_ESP8266, LWESP_MIN_AT_VERSION_PATCH_ESP8266)

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_HDR_H */
