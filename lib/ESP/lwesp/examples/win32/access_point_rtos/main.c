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
    espr_t res;

    printf("Starting ESP application!\r\n");

    /* Initialize ESP with default callback function */
    printf("Initializing ESP-AT Lib\r\n");
    if (esp_init(esp_callback_func, 1) != espOK) {
        printf("Cannot initialize ESP-AT Lib!\r\n");
    } else {
        printf("ESP-AT Lib initialized!\r\n");
    }

    /* Enable access point only mode */
    if ((res = esp_set_wifi_mode(ESP_MODE_AP, NULL, NULL, 1)) == espOK) {
        printf("ESP set to access-point-only mode\r\n");
    } else {
        printf("Problems setting ESP to access-point-only mode: %d\r\n", (int)res);
    }

    /* Configure access point */
    res = esp_ap_configure("ESP_AccessPoint", "ap_password", 13, ESP_ECN_WPA2_PSK, 5, 0, NULL, NULL, 1);
    if (res == espOK) {
        printf("Access point configured!\r\n");
    } else {
        printf("Cannot configure access point!\r\n");
    }

    /* The rest is handled in event function */

    /*
     * Do not stop program here.
     * New threads were created for ESP processing
     */
    while (1) {
        esp_delay(1000);
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
        case ESP_EVT_AP_CONNECTED_STA: {
            esp_mac_t* mac = esp_evt_ap_connected_sta_get_mac(evt);
            printf("New station connected to access point with MAC address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                (int)mac->mac[0], (int)mac->mac[1], (int)mac->mac[2],
                (int)mac->mac[3], (int)mac->mac[4], (int)mac->mac[5]
            );
            break;
        }
        case ESP_EVT_AP_IP_STA: {
            esp_mac_t* mac = esp_evt_ap_ip_sta_get_mac(evt);
            esp_ip_t* ip = esp_evt_ap_ip_sta_get_ip(evt);
            printf("IP %d.%d.%d.%d assigned to station with MAC address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                (int)ip->ip[0], (int)ip->ip[1], (int)ip->ip[2], (int)ip->ip[3],
                (int)mac->mac[0], (int)mac->mac[1], (int)mac->mac[2],
                (int)mac->mac[3], (int)mac->mac[4], (int)mac->mac[5]
            );
            break;
        }
        case ESP_EVT_AP_DISCONNECTED_STA: {
            esp_mac_t* mac = esp_evt_ap_disconnected_sta_get_mac(evt);
            printf("Station disconnected from access point with MAC address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                (int)mac->mac[0], (int)mac->mac[1], (int)mac->mac[2],
                (int)mac->mac[3], (int)mac->mac[4], (int)mac->mac[5]
            );
            break;
        }
        default: break;
    }
    return espOK;
}
