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
#include "netconn_client.h"

static espr_t esp_callback_func(esp_evt_t* evt);
static espr_t esp_conn_callback_func(esp_evt_t* evt);

#define CONN_HOST           "example.com"
#define CONN_PORT           80

/**
 * \brief           Minimal connection request
 */
static const uint8_t
request_data[] = ""
"GET / HTTP/1.1\r\n"
"Host: " CONN_HOST "\r\n"
"Connection: close\r\n"
"\r\n";

/**
 * \brief           Program entry point
 */
int
main(void) {
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

    /*
     * Start new connections to example.com
     *
     * Use non-blocking method and process further data in callback function
     */
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, CONN_PORT, NULL, esp_conn_callback_func, 0);
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, CONN_PORT, NULL, esp_conn_callback_func, 0);
    
    /*
     * An example of connection which should fail in connecting.
     * In this case, \ref ESP_EVT_CONN_ERROR event should be triggered
     * in callback function processing
     */
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, CONN_HOST, 10, NULL, esp_conn_callback_func, 0);

    /*
     * Do not stop program here as we still need to wait
     * for commands to be processed
     */
    while (1) {
        esp_delay(1000);
    }

    return 0;
}

/**
 * \brief           Callback function for connection events
 * \param[in]       evt: Event information with data
 * \return          espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_conn_callback_func(esp_evt_t* evt) {
    esp_conn_p conn;

    conn = esp_conn_get_from_evt(evt);          /* Get connection handle from event */
    switch (esp_evt_get_type(evt)) {
        case ESP_EVT_CONN_ACTIVE: {             /* Connection just active */
            printf("Connection %d active!\r\n", (int)esp_conn_getnum(conn));
            printf("Sending data on connection %d to remote server\r\n", (int)esp_conn_getnum(conn));
            esp_conn_send(conn, request_data, sizeof(request_data) - 1, NULL, 0);
            break;
        }
        case ESP_EVT_CONN_SEND: {               /* Data send event */
            espr_t res = esp_evt_conn_send_get_result(evt);
            if (res == espOK) {
                size_t len = esp_evt_conn_send_get_length(evt);
                printf("Successfully sent %d bytes on connection %d\r\n",
                    (int)len, (int)esp_conn_getnum(conn));
            } else {
                printf("Error trying to send data on connection %d\r\n",
                    (int)esp_conn_getnum(conn));
            }
            break;
        }
        case ESP_EVT_CONN_RECV: {               /* Connection data received */
            esp_pbuf_p p;
            p = esp_evt_conn_recv_get_buff(evt);/* Get received buffer */
            if (p != NULL) {
                printf("Connection %d data received with %d bytes\r\n",
                    (int)esp_conn_getnum(conn), (int)esp_pbuf_length(p, 1));
            }
            break;
        }
        case ESP_EVT_CONN_CLOSE: {              /* Connection closed */
            printf("Connection %d closed!\r\n", (int)esp_conn_getnum(conn));
            break;
        }
        case ESP_EVT_CONN_ERROR: {              /* Error connecting to server */
            const char* host = esp_evt_conn_error_get_host(evt);
            esp_port_t port = esp_evt_conn_error_get_port(evt);
            printf("Error connecting to %s:%d\r\n", host, (int)port);
            break;
        }
    }
    return espOK;
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