/**
 * \file            esp.c
 * \brief           Main ESP core file
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
#include "esp/esp_mem.h"
#include "esp/esp_threads.h"
#include "system/esp_ll.h"

/* Check for configuration */
#if ESP_CFG_OS != 1
#error ESP_CFG_OS must be set to 1 in current revision!
#endif /* ESP_CFG_OS != 1 */

#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
//#error ESP_CFG_CONN_MANUAL_TCP_RECEIVE must be set to 0 in current revision!
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */

static espr_t           def_callback(esp_evt_t* evt);
static esp_evt_func_t   def_evt_link;

esp_t esp;

/**
 * \brief           Default callback function for events
 * \param[in]       evt: Pointer to callback data structure
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
def_callback(esp_evt_t* evt) {
    ESP_UNUSED(evt);
    return espOK;
}

/**
 * \brief           Init and prepare ESP stack for device operation
 * \note            Function must be called from operating system thread context. 
 *                  It creates necessary threads and waits them to start, thus running operating system is important.
 *                  - When \ref ESP_CFG_RESET_ON_INIT is enabled, reset sequence will be sent to device
 *                      otherwise manual call to \ref esp_reset is required to setup device
 *                  - When \ref ESP_CFG_RESTORE_ON_INIT is enabled, restore sequence will be sent to device.
 *
 * \param[in]       evt_func: Global event callback function for all major events
 * \param[in]       blocking: Status whether command should be blocking or not.
 *                      Used when \ref ESP_CFG_RESET_ON_INIT or \ref ESP_CFG_RESTORE_ON_INIT are enabled.
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_init(esp_evt_fn evt_func, const uint32_t blocking) {
    espr_t res = espOK;

    esp.status.f.initialized = 0;               /* Clear possible init flag */

    def_evt_link.fn = evt_func != NULL ? evt_func : def_callback;
    esp.evt_func = &def_evt_link;               /* Set callback function */

    esp.evt_server = NULL;                      /* Set default server callback function */

    if (!esp_sys_init()) {                      /* Init low-level system */
        goto cleanup;
    }

    if (!esp_sys_sem_create(&esp.sem_sync, 1)) {/* Create sync semaphore between threads */
        ESP_DEBUGF(ESP_CFG_DBG_INIT | ESP_DBG_LVL_SEVERE | ESP_DBG_TYPE_TRACE,
            "[CORE] Cannot allocate sync semaphore!\r\n");
        goto cleanup;
    }

    /* Create message queues */
    if (!esp_sys_mbox_create(&esp.mbox_producer, ESP_CFG_THREAD_PRODUCER_MBOX_SIZE)) {  /* Producer */
        ESP_DEBUGF(ESP_CFG_DBG_INIT | ESP_DBG_LVL_SEVERE | ESP_DBG_TYPE_TRACE,
            "[CORE] Cannot allocate producer mbox queue!\r\n");
        goto cleanup;
    }
    if (!esp_sys_mbox_create(&esp.mbox_process, ESP_CFG_THREAD_PROCESS_MBOX_SIZE)) {  /* Process */
        ESP_DEBUGF(ESP_CFG_DBG_INIT | ESP_DBG_LVL_SEVERE | ESP_DBG_TYPE_TRACE,
            "[CORE] Cannot allocate process mbox queue!\r\n");
        goto cleanup;
    }

    /* Create threads */
    esp_sys_sem_wait(&esp.sem_sync, 0);         /* Lock semaphore */
    if (!esp_sys_thread_create(&esp.thread_produce, "esp_produce", esp_thread_produce, &esp.sem_sync, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO)) {
        ESP_DEBUGF(ESP_CFG_DBG_INIT | ESP_DBG_LVL_SEVERE | ESP_DBG_TYPE_TRACE,
            "[CORE] Cannot create producing thread!\r\n");
        esp_sys_sem_release(&esp.sem_sync);     /* Release semaphore and return */
        goto cleanup;
    }
    esp_sys_sem_wait(&esp.sem_sync, 0);         /* Wait semaphore, should be unlocked in process thread */
    if (!esp_sys_thread_create(&esp.thread_process, "esp_process", esp_thread_process, &esp.sem_sync, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO)) {
        ESP_DEBUGF(ESP_CFG_DBG_INIT | ESP_DBG_LVL_SEVERE | ESP_DBG_TYPE_TRACE,
            "[CORE] Cannot allocate processing thread!\r\n");
        esp_sys_thread_terminate(&esp.thread_produce);  /* Delete produce thread */
        esp_sys_sem_release(&esp.sem_sync);     /* Release semaphore and return */
        goto cleanup;
    }
    esp_sys_sem_wait(&esp.sem_sync, 0);         /* Wait semaphore, should be unlocked in produce thread */
    esp_sys_sem_release(&esp.sem_sync);         /* Release semaphore manually */

    esp_core_lock();
    esp.ll.uart.baudrate = ESP_CFG_AT_PORT_BAUDRATE;/* Set default baudrate value */
    esp_ll_init(&esp.ll);                       /* Init low-level communication */

#if !ESP_CFG_INPUT_USE_PROCESS
    esp_buff_init(&esp.buff, ESP_CFG_RCV_BUFF_SIZE);    /* Init buffer for input data */
#endif /* !ESP_CFG_INPUT_USE_PROCESS */

    esp.status.f.initialized = 1;               /* We are initialized now */
    esp.status.f.dev_present = 1;               /* We assume device is present at this point */

    espi_send_cb(ESP_EVT_INIT_FINISH);          /* Call user callback function */

    /*
     * Call reset command and call default
     * AT commands to prepare basic setup for device
     */
    espi_conn_init();                           /* Init connection module */

#if ESP_CFG_RESTORE_ON_INIT
    if (esp.status.f.dev_present) {             /* In case device exists */
        esp_core_unlock();
        res = esp_restore(NULL, NULL, blocking);/* Restore device */
        esp_core_lock();
    }
#endif /* ESP_CFG_RESTORE_ON_INIT */
#if ESP_CFG_RESET_ON_INIT
    if (esp.status.f.dev_present) {
        esp_core_unlock();
        res = esp_reset_with_delay(ESP_CFG_RESET_DELAY_DEFAULT, NULL, NULL, blocking);  /* Send reset sequence with delay */
        esp_core_lock();
    }
#endif /* ESP_CFG_RESET_ON_INIT */
    ESP_UNUSED(blocking);                       /* Prevent compiler warnings */
    esp_core_unlock();

    return res;

cleanup:
    if (esp_sys_mbox_isvalid(&esp.mbox_producer)) {
        esp_sys_mbox_delete(&esp.mbox_producer);
        esp_sys_mbox_invalid(&esp.mbox_producer);
    }
    if (esp_sys_mbox_isvalid(&esp.mbox_process)) {
        esp_sys_mbox_delete(&esp.mbox_process);
        esp_sys_mbox_invalid(&esp.mbox_process);
    }
    if (esp_sys_sem_isvalid(&esp.sem_sync)) {
        esp_sys_sem_delete(&esp.sem_sync);
        esp_sys_sem_invalid(&esp.sem_sync);
    }
    return espERRMEM;
}

/**
 * \brief           Execute reset and send default commands
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_reset(const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    return esp_reset_with_delay(0, evt_fn, evt_arg, blocking);
}

/**
 * \brief           Execute reset and send default commands with delay before first command
 * \param[in]       delay: Number of milliseconds to wait before initiating first command to device
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_reset_with_delay(uint32_t delay,
                        const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_RESET;
    ESP_MSG_VAR_REF(msg).msg.reset.delay = delay;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 5000);
}

/**
 * \brief           Execute restore command and set module to default values
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_restore(const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_RESTORE;
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_RESET;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 5000);
}

/**
 * \brief           Sets WiFi mode to either station only, access point only or both
 *
 * Configuration changes will be saved in the NVS area of ESP device.
 *
 * \param[in]       mode: Mode of operation. This parameter can be a value of \ref esp_mode_t enumeration
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_wifi_mode(esp_mode_t mode,
                    const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWMODE;
    ESP_MSG_VAR_REF(msg).msg.wifi_mode.mode = mode;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Sets baudrate of AT port (usually UART)
 * \param[in]       baud: Baudrate in units of bits per second
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_at_baudrate(uint32_t baud,
                    const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_UART;
    ESP_MSG_VAR_REF(msg).msg.uart.baudrate = baud;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 2000);
}

/**
 * \brief           Enables or disables server mode
 * \param[in]       en: Set to `1` to enable server, `0` otherwise
 * \param[in]       port: Port number used to listen on. Must also be used when disabling server mode
 * \param[in]       max_conn: Number of maximal connections populated by server
 * \param[in]       timeout: Time used to automatically close the connection in units of seconds.
 *                      Set to `0` to disable timeout feature (not recommended)
 * \param[in]       server_evt_fn: Connection callback function for new connections started as server
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_set_server(uint8_t en, esp_port_t port, uint16_t max_conn, uint16_t timeout, esp_evt_fn server_evt_fn,
                const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("port > 0", port > 0);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSERVER;
    if (en) {
        ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSERVERMAXCONN;  /* First command is to set maximal number of connections for server */
    }
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.en = en;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.port = port;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.max_conn = max_conn;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.timeout = timeout;
    ESP_MSG_VAR_REF(msg).msg.tcpip_server.cb = server_evt_fn;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

#if ESP_CFG_MODE_STATION || __DOXYGEN__

/**
 * \brief           Update ESP software remotely
 * \note            ESP must be connected to access point to use this feature
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_update_sw(const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIUPDATE;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 180000);
}

#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */

/**
 * \brief           Lock stack from multi-thread access, enable atomic access to core
 *
 * If lock was `0` prior funcion call, lock is enabled and increased
 *
 * \note            Function may be called multiple times to increase locks.
 *                  Application must take care to call \ref esp_core_unlock
 *                  the same amount of time to make sure lock gets back to `0`
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_core_lock(void) {
    esp_sys_protect();
    esp.locked_cnt++;
    return espOK;
}

/**
 * \brief           Unlock stack for multi-thread access
 *
 * Used in conjunction with \ref esp_core_lock function
 *
 * If lock was non-zero before function call, lock is decreased.
 * When `lock == 0`, protection is disabled and other threads may access to core
 *
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_core_unlock(void) {
    esp.locked_cnt--;
    esp_sys_unprotect();
    return espOK;
}

/**
 * \brief           Notify stack if device is present or not
 *
 * Use this function to notify stack that device is not physically connected
 * and not ready to communicate with host device
 *
 * \param[in]       present: Flag indicating device is present
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_device_set_present(uint8_t present,
                        const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    espr_t res = espOK;
    esp_core_lock();
    present = present ? 1 : 0;
    if (present != esp.status.f.dev_present) {
        esp.status.f.dev_present = present;

        if (!esp.status.f.dev_present) {
            /* Manually reset stack to default device state */
            espi_reset_everything(1);
        } else {
#if ESP_CFG_RESET_ON_DEVICE_PRESENT
            esp_core_unlock();
            res = esp_reset_with_delay(ESP_CFG_RESET_DELAY_DEFAULT, evt_fn, evt_arg, blocking);
            esp_core_lock();
#endif /* ESP_CFG_RESET_ON_DEVICE_PRESENT */
        }
        espi_send_cb(ESP_EVT_DEVICE_PRESENT);       /* Send present event */
    }
    esp_core_unlock();

    ESP_UNUSED(evt_fn);
    ESP_UNUSED(evt_arg);
    ESP_UNUSED(blocking);

    return res;
}

/**
 * \brief           Check if device is present
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_device_is_present(void) {
    uint8_t res;
    esp_core_lock();
    res = esp.status.f.dev_present;
    esp_core_unlock();
    return res;
}

#if ESP_CFG_ESP8266 || __DOXYGEN__

/**
 * \brief           Check if modem device is ESP8266
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_device_is_esp8266(void) {
    uint8_t res;
    esp_core_lock();
    res = esp.status.f.dev_present && esp.m.device == ESP_DEVICE_ESP8266;
    esp_core_unlock();
    return res;
}

#endif /* ESP_CFG_ESP8266 || __DOXYGEN__ */

#if ESP_CFG_ESP32 || __DOXYGEN__

/**
 * \brief           Check if modem device is ESP32
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_device_is_esp32(void) {
    uint8_t res;
    esp_core_lock();
    res = esp.status.f.dev_present && esp.m.device == ESP_DEVICE_ESP32;
    esp_core_unlock();
    return res;
}

#endif /* ESP_CFG_ESP32 || __DOXYGEN__ */

/**
 * \brief           Get current AT firmware version of connected device
 * \param[out]      version: Output version variable
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_get_current_at_fw_version(esp_sw_version_t* const version) {
    ESP_MEMCPY(version, &esp.m.version_at, sizeof(*version));
    return 1;
}

/**
 * \brief           Delay for amount of milliseconds
 *
 * Delay is based on operating system semaphores. 
 * It locks semaphore and waits for timeout in `ms` time.
 * Based on operating system, thread may be put to \e blocked list during delay and may improve execution speed
 *
 * \param[in]       ms: Milliseconds to delay
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_delay(const uint32_t ms) {
    esp_sys_sem_t sem;
    if (ms == 0) {
        return 1;
    }
    if (esp_sys_sem_create(&sem, 0)) {
        esp_sys_sem_wait(&sem, ms);
        esp_sys_sem_release(&sem);
        esp_sys_sem_delete(&sem);
        return 1;
    }
    return 0;
}
