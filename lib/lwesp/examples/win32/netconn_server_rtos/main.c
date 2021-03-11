/**
 * \file            main.c
 * \brief           Main file
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
 * Before you start using WIN32 implementation with USB and VCP,
 * check lwesp_ll_win32.c implementation and choose your COM port!
 */
#include "lwesp/lwesp.h"
#include "station_manager.h"
#include "netconn_server.h"

static lwespr_t lwesp_callback_func(lwesp_evt_t* evt);

/**
 * \brief           Program entry point
 */
int
main(void) {
    printf("Starting ESP application!\r\n");

    /* Initialize ESP with default callback function */
    printf("Initializing LwESP\r\n");
    if (lwesp_init(lwesp_callback_func, 1) != lwespOK) {
        printf("Cannot initialize LwESP!\r\n");
    } else {
        printf("LwESP initialized!\r\n");
    }

    /*
     * Connect to access point.
     *
     * Try unlimited time until access point accepts up.
     * Check for station_manager.c to define preferred access points ESP should connect to
     */
    connect_to_preferred_access_point(1);

    /*
     * Start netconn server processing in separate thread
     *
     * Use netconn_server_thread function as thread entry point
     */
    lwesp_sys_thread_create(NULL, "netconn_client", (lwesp_sys_thread_fn)netconn_server_thread, NULL, 0, 0);

    /*
     * Do not stop program here.
     * New threads were created for ESP processing
     */
    while (1) {
        lwesp_delay(1000);
    }

    return 0;
}

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
lwesp_callback_func(lwesp_evt_t* evt) {
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            lwesp_sw_version_t v_min, v_curr;

            lwesp_get_min_at_fw_version(&v_min);
            lwesp_get_current_at_fw_version(&v_curr);

            printf("Current ESP8266 AT version is not supported by library!\r\n");
            printf("Minimum required AT version is: %d.%d.%d\r\n", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
            printf("Current AT version is: %d.%d.%d\r\n", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
            break;
        }
        case LWESP_EVT_INIT_FINISH: {
            printf("Library initialized!\r\n");
            break;
        }
        case LWESP_EVT_RESET_DETECTED: {
            printf("Device reset detected!\r\n");
            break;
        }
        default: break;
    }
    return lwespOK;
}
