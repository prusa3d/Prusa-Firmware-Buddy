/**
 * \file            esp_ll_win32.c
 * \brief           Low-level communication with ESP device for WIN32
 */

/*
 * Copyright (c) 2018 Tilen Majerle
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
 */
#include "system/esp_ll.h"
#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"

#if !__DOXYGEN__

static uint8_t initialized = 0;

DWORD thread_id;
HANDLE thread_handle;
static void uart_thread(void* param);
HANDLE comPort;                                 /*!< COM port handle */
uint8_t data_buffer[0x1000];                    /*!< Received data array */

/**
 * \brief           Send data to ESP device, function called from ESP stack when we have data to send
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
send_data(const void* data, size_t len) {
    if (comPort != NULL) {
        WriteFile(comPort, data, len, NULL, NULL);
        return len;
    }
    return 0;
}

/**
 * \brief           Configure UART (USB to UART)
 */
static void
configure_uart(uint32_t baudrate) {
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(dcb);

    /*
     * On first call,
     * create virtual file on selected COM port and open it
     * as generic read and write
     */
    if (!initialized) {
        comPort = CreateFile(L"\\\\.\\COM6",
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            OPEN_EXISTING,
            0,
            NULL
        );
    }

    /* Configure COM port parameters */
    if (GetCommState(comPort, &dcb)) {
        COMMTIMEOUTS timeouts;

        dcb.BaudRate = baudrate;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;

        if (!SetCommState(comPort, &dcb)) {
            printf("Cannot set COM PORT info\r\n");
        }
        if (GetCommTimeouts(comPort, &timeouts)) {
            /* Set timeout to return immediatelly from ReadFile function */
            timeouts.ReadIntervalTimeout = MAXDWORD;
            timeouts.ReadTotalTimeoutConstant = 0;
            timeouts.ReadTotalTimeoutMultiplier = 0;
            if (!SetCommTimeouts(comPort, &timeouts)) {
                printf("Cannot set COM PORT timeouts\r\n");
            }
            GetCommTimeouts(comPort, &timeouts);
        } else {
            printf("Cannot get COM PORT timeouts\r\n");
        }
    } else {
        printf("Cannot get COM PORT info\r\n");
    }

    /* On first function call, create a thread to read data from COM port */
    if (!initialized) {
        thread_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)uart_thread, NULL, 0, 0);
    }
}

/**
 * \brief           UART thread
 */
static void
uart_thread(void* param) {
    DWORD bytes_read;
    esp_sys_sem_t sem;
    FILE* file = NULL;

    while (comPort == NULL);

    esp_sys_sem_create(&sem, 0);                /* Create semaphore for delay functions */

    fopen_s(&file, "log_file.txt", "w+");       /* Open debug file in write mode */
    while (1) {
        /*
         * Try to read data from COM port
         * and send it to upper layer for processing
         */
        do {
            ReadFile(comPort, data_buffer, sizeof(data_buffer), &bytes_read, NULL);
            if (bytes_read > 0) {
                for (DWORD i = 0; i < bytes_read; i++) {
                    printf("%c", data_buffer[i]);
                }

                /* Send received data to input processing module */
#if ESP_CFG_INPUT_USE_PROCESS
                esp_input_process(data_buffer, (size_t)bytes_read);
#else
                esp_input(data_buffer, (size_t)bytes_read);
#endif

                /* Write received data to output debug file */
                if (file != NULL) {
                    fwrite(data_buffer, 1, bytes_read, file);
                    fflush(file);
                }
            }
        } while (bytes_read == (DWORD)sizeof(data_buffer));

        /* Implement delay to allow other tasks processing */
        esp_sys_sem_wait(&sem, 1);
    }
}

/**
 * \brief           Callback function called from initialization process
 *
 * \note            This function may be called multiple times if AT baudrate is changed from application.
 *                  It is important that every configuration except AT baudrate is configured only once!
 *
 * \note            This function may be called from different threads in ESP stack when using OS.
 *                  When \ref ESP_CFG_INPUT_USE_PROCESS is set to 1, this function may be called from user UART thread.
 *
 * \param[in,out]   ll: Pointer to \ref esp_ll_t structure to fill data for communication functions
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
    /* Step 1: Configure memory for dynamic allocations */
    static uint8_t memory[0x10000];             /* Create memory for dynamic allocations with specific size */

    /*
     * Create memory region(s) of memory.
     * If device has internal/external memory available,
     * multiple memories may be used
     */
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));  /* Assign memory for allocations to ESP library */
    }

    /* Step 2: Set AT port send function to use when we have data to transmit */
    if (!initialized) {
        ll->send_fn = send_data;                /* Set callback function to send data */
    }

    /* Step 3: Configure AT port to be able to send/receive data to/from ESP device */
    configure_uart(ll->uart.baudrate);          /* Initialize UART for communication */
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
    if (thread_handle != NULL) {
        CloseHandle(thread_handle);             /* Close handle */
        thread_handle = NULL;
    }
    initialized = 0;                            /* Clear initialized flag */
    return espOK;
}

#endif /* !__DOXYGEN__ */
