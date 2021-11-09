/**
 * \file            esp_sys_freertos_os.h
 * \brief           FreeRTOS native port
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
 * Author:          Adrian Carpenter (FreeRTOS port)
 */
#ifndef ESP_HDR_SYSTEM_FREERTOS_OS_H
#define ESP_HDR_SYSTEM_FREERTOS_OS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdlib.h>

#include "esp_config.h"

#if ESP_CFG_OS && !__DOXYGEN__
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

typedef SemaphoreHandle_t           esp_sys_mutex_t;
typedef SemaphoreHandle_t           esp_sys_sem_t;
typedef QueueHandle_t               esp_sys_mbox_t;
typedef TaskHandle_t                esp_sys_thread_t;
typedef UBaseType_t                 esp_sys_thread_prio_t;

#define ESP_SYS_MBOX_NULL           (QueueHandle_t)0
#define ESP_SYS_SEM_NULL            (SemaphoreHandle_t)0
#define ESP_SYS_MUTEX_NULL          (SemaphoreHandle_t)0
#define ESP_SYS_TIMEOUT             ((TickType_t)portMAX_DELAY)
#define ESP_SYS_THREAD_PRIO         (1)
#define ESP_SYS_THREAD_SS           (1024)
#endif /* ESP_CFG_OS && !__DOXYGEN__ */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ESP_HDR_SYSTEM_FREERTOS_OS_H */
