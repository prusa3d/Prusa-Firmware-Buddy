/**
 * \file            esp_threads.c
 * \brief           OS threads implementations
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
#include "esp/esp_threads.h"
#include "esp/esp_parser.h"
#include "esp/esp_int.h"
#include "esp/esp_timeout.h"
#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "system/esp_sys.h"

/**
 * \brief           User thread to process input packets from API functions
 * \param[in]       arg: User argument. Semaphore to release when thread starts
 */
void
esp_thread_produce(void* const arg) {
    esp_sys_sem_t* sem = arg;
    esp_t* e = &esp;
    esp_msg_t* msg;
    espr_t res;
    uint32_t time;

    /* Thread is running, unlock semaphore */
    if (esp_sys_sem_isvalid(sem)) {
        esp_sys_sem_release(sem);               /* Release semaphore */
    }

    esp_core_lock();
    while (1) {
        esp_core_unlock();
        do {
            time = esp_sys_mbox_get(&e->mbox_producer, (void **)&msg, 0);   /* Get message from queue */
        } while (time == ESP_SYS_TIMEOUT || msg == NULL);
        ESP_THREAD_PRODUCER_HOOK();             /* Execute producer thread hook */
        esp_core_lock();

        res = espOK;                            /* Start with OK */
        e->msg = msg;                           /* Set message handle */

        /*
         * This check is performed when adding command to queue
         * Do it again here to prevent long timeouts,
         * if device present flag changes
         */
        if (!e->status.f.dev_present) {
            res = espERRNODEVICE;
        }

        /* For reset message, we can have delay! */
        if (res == espOK && msg->cmd_def == ESP_CMD_RESET) {
            if (msg->msg.reset.delay > 0) {
                esp_delay(msg->msg.reset.delay);
            }
            espi_reset_everything(1);           /* Reset stack before trying to reset */
        }

        /*
         * Try to call function to process this message
         * Usually it should be function to transmit data to AT port
         */
        if (res == espOK && msg->fn != NULL) {  /* Check for callback processing function */
            /* 
             * Obtain semaphore 
             * This code should not block at any point.
             * If it blocks, severe problems occurred and program should 
             * immediate terminate
             */
            esp_core_unlock();
            esp_sys_sem_wait(&e->sem_sync, 0);  /* First call */
            esp_core_lock();
            res = msg->fn(msg);                 /* Process this message, check if command started at least */
            time = ~ESP_SYS_TIMEOUT;            /* Reset time */
            if (res == espOK) {                 /* We have valid data and data were sent */
                esp_core_unlock();
                time = esp_sys_sem_wait(&e->sem_sync, msg->block_time); /* Second call; Wait for synchronization semaphore from processing thread or timeout */
                esp_core_lock();
                if (time == ESP_SYS_TIMEOUT) {  /* Sync timeout occurred? */
                    res = espTIMEOUT;           /* Timeout on command */
                }
            }

            /* Notify application on command timeout */
            if (res == espTIMEOUT) {
                espi_send_cb(ESP_EVT_CMD_TIMEOUT);
            }

            ESP_DEBUGW(ESP_CFG_DBG_THREAD | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_SEVERE,
                res == espTIMEOUT,
                "[THREAD] Timeout in produce thread waiting for command to finish in process thread\r\n");
            ESP_DEBUGW(ESP_CFG_DBG_THREAD | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_SEVERE,
                res != espOK && res != espTIMEOUT,
                "[THREAD] Could not start execution for command %d\r\n", (int)msg->cmd);

            /*
             * Manually release semaphore in all cases:
             *
             * Case 1: msg->fn function fails, command did not start,
             *           application needs to release previously acquired semaphore
             * Case 2: If time == TIMEOUT, acquiring on second call was not successful,
             *           application has to manually release semaphore, taken on first call
             * Case 3: If time != TIMEOUT, acquiring on second call was successful,
             *           which effectively means that another thread successfully released semaphore,
             *           application has to release semaphore, now taken on second call
             *
             * If application would not manually release semaphore,
             * and if command would return with timeout (or fail),
             * it would not be possible to start a new command after,
             * because semaphore would be still locked
             */
            esp_sys_sem_release(&e->sem_sync);
        } else {
            if (res == espOK) {
                res = espERR;                   /* Simply set error message */
            }
        }
        if (res != espOK) {
            /* Process global callbacks */
            espi_process_events_for_timeout_or_error(msg, res);

            msg->res = res;                     /* Save response */
        }

#if ESP_CFG_USE_API_FUNC_EVT
        /* Send event function to user */
        if (msg->evt_fn != NULL) {
            msg->evt_fn(msg->res, msg->evt_arg);/* Send event with user argument */
        }
#endif /* ESP_CFG_USE_API_FUNC_EVT */

        /*
         * In case message is blocking,
         * release semaphore and notify finished with processing
         * otherwise directly free memory of message structure
         */
        if (msg->is_blocking) {
            esp_sys_sem_release(&msg->sem);
        } else {
            ESP_MSG_VAR_FREE(msg);
        }
        e->msg = NULL;
    }
}

/**
 * \brief           Thread for processing received data from device
 *
 *                  This thread is also used to handle timeout events
 *                  in correct time order as it is never blocked by user command
 *
 * \param[in]       arg: User argument. Semaphore to release when thread starts
 * \sa              ESP_CFG_INPUT_USE_PROCESS
 */
void
esp_thread_process(void* const arg) {
    esp_sys_sem_t* sem = arg;
    esp_t* e = &esp;
    esp_msg_t* msg;
    uint32_t time;

    /* Thread is running, unlock semaphore */
    if (esp_sys_sem_isvalid(sem)) {
        esp_sys_sem_release(sem);               /* Release semaphore */
    }

#if !ESP_CFG_INPUT_USE_PROCESS
    esp_core_lock();
    while (1) {
        esp_core_unlock();
        time = espi_get_from_mbox_with_timeout_checks(&e->mbox_process, (void **)&msg, 10);
        ESP_THREAD_PROCESS_HOOK();              /* Execute process thread hook */
        esp_core_lock();

        if (time == ESP_SYS_TIMEOUT || msg == NULL) {
            ESP_UNUSED(time);                   /* Unused variable */
        }
        espi_process_buffer();                  /* Process input data */
#else /* ESP_CFG_INPUT_USE_PROCESS */
    while (1) {
        /*
         * Check for next timeout event only here
         *
         * If there are no timeouts to process, we can wait unlimited time.
         * In case new timeout occurs, thread will wake up by writing new element to mbox process queue
         */
        time = espi_get_from_mbox_with_timeout_checks(&e->mbox_process, (void **)&msg, 0);
        ESP_THREAD_PROCESS_HOOK();              /* Execute process thread hook */
        ESP_UNUSED(time);
#endif /* !ESP_CFG_INPUT_USE_PROCESS */
    }
}
