/**
 * \file            esp_sys_win32.h
 * \brief           WIN32 based system file implementation
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
#ifndef ESP_HDR_SYSTEM_WIN32_H
#define ESP_HDR_SYSTEM_WIN32_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdint.h"
#include "stdlib.h"

#include "esp_config.h"

#if ESP_CFG_OS && !__DOXYGEN__

#include "windows.h"

typedef HANDLE                      esp_sys_mutex_t;
typedef HANDLE                      esp_sys_sem_t;
typedef HANDLE                      esp_sys_mbox_t;
typedef HANDLE                      esp_sys_thread_t;
typedef int                         esp_sys_thread_prio_t;
#define ESP_SYS_MBOX_NULL           (HANDLE)0
#define ESP_SYS_SEM_NULL            (HANDLE)0
#define ESP_SYS_MUTEX_NULL          (HANDLE)0
#define ESP_SYS_TIMEOUT             (INFINITE)
#define ESP_SYS_THREAD_PRIO         (0)
#define ESP_SYS_THREAD_SS           (1024)

#endif /* ESP_CFG_OS && !__DOXYGEN__ */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ESP_HDR_SYSTEM_WIN32_H */
