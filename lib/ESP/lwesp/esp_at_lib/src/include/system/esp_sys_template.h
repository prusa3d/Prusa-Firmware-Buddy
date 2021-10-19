/**
 * \file            esp_sys_template.h
 * \brief           Template file for system functions
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
#ifndef ESP_HDR_SYSTEM_H
#define ESP_HDR_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdlib.h>

#include "esp_config.h"

/**
 * \addtogroup      ESP_SYS
 * \{
 */

#if ESP_CFG_OS || __DOXYGEN__
#include "cmsis_os.h"

/**
 * \brief           ESP system mutex ID type
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
typedef osMutexId           esp_sys_mutex_t;

/**
 * \brief           ESP system semaphore ID type
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
typedef osSemaphoreId       esp_sys_sem_t;

/**
 * \brief           ESP system message queue ID type
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
typedef osMessageQId        esp_sys_mbox_t;

/**
 * \brief           ESP system thread ID type
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
typedef osThreadId          esp_sys_thread_t;

/**
 * \brief           ESP system thread priority type
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
typedef osPriority          esp_sys_thread_prio_t;

/**
 * \brief           Value indicating message queue is not valid
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
#define ESP_SYS_MBOX_NULL           (osMessageQId)0

/**
 * \brief           Value indicating semaphore is not valid
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
#define ESP_SYS_SEM_NULL            (osSemaphoreId)0

/**
 * \brief           Value indicating mutex is not valid
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
#define ESP_SYS_MUTEX_NULL          (osMutexId)0

/**
 * \brief           Value indicating timeout for OS timings
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
#define ESP_SYS_TIMEOUT             ((uint32_t)osWaitForever)

/**
 * \brief           ESP stack threads priority parameter
 * \note            Usually normal priority is ok. If many threads are in the system and high traffic is introduced
 *                  This value might need to be set to higher value
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
#define ESP_SYS_THREAD_PRIO         (osPriorityNormal)

/**
 * \brief           Stack size of system threads
 * \note            Keep as is in case of CMSIS based OS, otherwise change for your OS
 */
#define ESP_SYS_THREAD_SS           (1024)
#endif /* ESP_OS || __DOXYGEN__ */

/**
 * \}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ESP_HDR_SYSTEM_H */
