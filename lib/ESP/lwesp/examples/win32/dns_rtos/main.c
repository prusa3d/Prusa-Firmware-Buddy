/**
 * \file            main.c
 * \brief           Main file
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
 * Before you start using WIN32 implementation with USB and VCP,
 * check esp_ll_win32.c implementation and choose your COM port!
 */
#include "esp/esp.h"
#include "station_manager.h"

static espr_t esp_callback_func(esp_evt_t* evt);

/**
 * \brief           Program entry point
 */
int
main(void) {
    esp_ip_t ip;
    printf("Starting ESP application!\r\n");

    /* Initialize ESP with default callback function */
    printf("Initializing ESP-AT Lib\r\n");
    if (esp_init(esp_callback_func, 1) != espOK) {
        printf("Cannot initialize ESP-AT Lib!\r\n");
    } else {
        printf("ESP-AT Lib initialized!\r\n");
    }

    /*
     * Connect to access point.
     *
     * Try unlimited time until access point accepts up.
     * Check for station_manager.c to define preferred access points ESP should connect to
     */
    connect_to_preferred_access_point(1);

    /* Use DNS protocol to get IP address of domain name */
    if (esp_dns_gethostbyname("example.com", &ip, NULL, NULL, 1) == espOK) {
        printf("DNS record for example.com: %d.%d.%d.%d\r\n",
            (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
    } else {
        printf("Error on DNS resolver\r\n");
    }

    return 0;
}

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_callback_func(esp_evt_t* evt) {
    switch (esp_evt_get_type(evt)) {
        case ESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            esp_sw_version_t v_min, v_curr;

            esp_get_min_at_fw_version(&v_min);
            esp_get_current_at_fw_version(&v_curr);

            printf("Current ESP8266 AT version is not supported by library!\r\n");
            printf("Minimum required AT version is: %d.%d.%d\r\n", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
            printf("Current AT version is: %d.%d.%d\r\n", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
            break;
        }
        case ESP_EVT_INIT_FINISH: {
            printf("Library initialized!\r\n");
            break;
        }
        case ESP_EVT_RESET_DETECTED: {
            printf("Device reset detected!\r\n");
            break;
        }
        default: break;
    }
    return espOK;
}
