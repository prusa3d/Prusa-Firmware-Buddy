/**
 * \file            esp_ll_template.c
 * \brief           Low-level communication with ESP device template
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
#include "system/esp_ll.h"
#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"

static uint8_t initialized = 0;

/**
 * \brief           Send data to ESP device, function called from ESP stack when we have data to send
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
send_data(const void* data, size_t len) {
    /* Implement send function here */


    return len;                                 /* Return number of bytes actually sent to AT port */
}

/**
 * \brief           Callback function called from initialization process
 *
 * \note            This function may be called multiple times if AT baudrate is changed from application.
 *                  It is important that every configuration except AT baudrate is configured only once!
 *
 * \note            This function may be called from different threads in ESP stack when using OS.
 *                  When \ref ESP_CFG_INPUT_USE_PROCESS is set to `1`, this function may be called from user UART thread.
 *
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
#if !ESP_CFG_MEM_CUSTOM
    /* Step 1: Configure memory for dynamic allocations */
    static uint8_t memory[0x10000];             /* Create memory for dynamic allocations with specific size */

    /*
     * Create region(s) of memory.
     * If device has internal/external memory available,
     * multiple memories may be used
     */
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations to ESP library */
    }
#endif /* !ESP_CFG_MEM_CUSTOM */

    /* Step 2: Set AT port send function to use when we have data to transmit */
    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
    }

    /* Step 3: Configure AT port to be able to send/receive data to/from ESP device */
    configure_uart(->uart.baudrate);            /* Initialize UART for communication */
    initialized = 1;
    return espOK;
}

/**
 * \brief           Callback function to de-init low-level communication part
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ll_deinit(esp_ll_t* ll) {
    initialized = 0;                            /* Clear initialized flag */
    return espOK;
}
