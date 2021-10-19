/**
 * \file            esp_sys_cmsis_os2.c
 * \brief           System dependant functions for CMSIS based operating system
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
#include "system/esp_sys.h"
#include "cmsis_os2.h"

#if !__DOXYGEN__
static osMutexId_t sys_mutex;

uint8_t
esp_sys_init(void) {
    esp_sys_mutex_create(&sys_mutex);
    return 1;
}

uint32_t
esp_sys_now(void) {
    return osKernelGetTickCount();
}

#if ESP_CFG_OS

uint8_t
esp_sys_protect(void) {
    esp_sys_mutex_lock(&sys_mutex);
    return 1;
}

uint8_t
esp_sys_unprotect(void) {
    esp_sys_mutex_unlock(&sys_mutex);
    return 1;
}

uint8_t
esp_sys_mutex_create(esp_sys_mutex_t* p) {
    const osMutexAttr_t attr = {
        .attr_bits = osMutexRecursive
    };

    *p = osMutexNew(&attr);
    return *p != NULL;
}

uint8_t
esp_sys_mutex_delete(esp_sys_mutex_t* p) {
    return osMutexDelete(*p) == osOK;
}

uint8_t
esp_sys_mutex_lock(esp_sys_mutex_t* p) {
    return osMutexAcquire(*p, osWaitForever) == osOK;
}

uint8_t
esp_sys_mutex_unlock(esp_sys_mutex_t* p) {
    return osMutexRelease(*p);
}

uint8_t
esp_sys_mutex_isvalid(esp_sys_mutex_t* p) {
    return p != NULL && *p != NULL;
}

uint8_t
esp_sys_mutex_invalid(esp_sys_mutex_t* p) {
    *p = ESP_SYS_MUTEX_NULL;
    return 1;
}

uint8_t
esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt) {
    *p = osSemaphoreNew(1, cnt > 0, NULL);
    return *p != NULL;
}

uint8_t
esp_sys_sem_delete(esp_sys_sem_t* p) {
    return osSemaphoreDelete(*p) == osOK;
}

uint32_t
esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout) {
    uint32_t tick = osKernelGetTickCount();
    return (osSemaphoreAcquire(*p, !timeout ? osWaitForever : timeout) == osOK) ? (osKernelGetTickCount() - tick) : ESP_SYS_TIMEOUT;
}

uint8_t
esp_sys_sem_release(esp_sys_sem_t* p) {
    return osSemaphoreRelease(*p) == osOK;
}

uint8_t
esp_sys_sem_isvalid(esp_sys_sem_t* p) {
    return p != NULL && *p != NULL;
}

uint8_t
esp_sys_sem_invalid(esp_sys_sem_t* p) {
    *p = ESP_SYS_SEM_NULL;
    return 1;
}

uint8_t
esp_sys_mbox_create(esp_sys_mbox_t* b, size_t size) {
    *b = osMessageQueueNew(size, sizeof(void *), NULL);
    return *b != NULL;
}

uint8_t
esp_sys_mbox_delete(esp_sys_mbox_t* b) {
    if (osMessageQueueGetCount(*b)) {
        return 0;
    }
    return osMessageQueueDelete(*b) == osOK;
}

uint32_t
esp_sys_mbox_put(esp_sys_mbox_t* b, void* m) {
    uint32_t tick = osKernelGetTickCount();
    return osMessageQueuePut(*b, m, 0, osWaitForever) == osOK ? (osKernelGetTickCount() - tick) : ESP_SYS_TIMEOUT;
}

uint32_t
esp_sys_mbox_get(esp_sys_mbox_t* b, void** m, uint32_t timeout) {
    osStatus_t evt;
    uint32_t time = osKernelGetTickCount();

    evt = osMessageQueueGet(*b, *m, NULL, !timeout ? osWaitForever : timeout);
    if (evt == osOK) {
        return osKernelGetTickCount() - time;
    }
    return ESP_SYS_TIMEOUT;
}

uint8_t
esp_sys_mbox_putnow(esp_sys_mbox_t* b, void* m) {
    return osMessageQueuePut(*b, m, 0, 0) == osOK;
}

uint8_t
esp_sys_mbox_getnow(esp_sys_mbox_t* b, void** m) {
    osStatus_t evt;

    evt = osMessageQueueGet(*b, *m, 0, 0);
    return evt == osOK;
}

uint8_t
esp_sys_mbox_isvalid(esp_sys_mbox_t* b) {
    return b != NULL && *b != NULL;
}

uint8_t
esp_sys_mbox_invalid(esp_sys_mbox_t* b) {
    *b = ESP_SYS_MBOX_NULL;
    return 1;
}

uint8_t
esp_sys_thread_create(esp_sys_thread_t* t, const char* name, esp_sys_thread_fn thread_func, void* const arg, size_t stack_size, esp_sys_thread_prio_t prio) {
    esp_sys_thread_t id;
    const osThreadAttr_t attr = {
            .name = name,
            .priority = prio,
            .stack_size = stack_size > 0 ? stack_size : ESP_SYS_THREAD_SS
    };
    id = osThreadNew(thread_func, arg, &attr);
    if (t != NULL) {
        *t = id;
    }
    return id != NULL;
}

uint8_t
esp_sys_thread_terminate(esp_sys_thread_t* t) {
    osThreadTerminate(t != NULL ? *t : NULL);
    return 1;
}

uint8_t
esp_sys_thread_yield(void) {
    osThreadYield();
    return 1;
}

#endif /* ESP_CFG_OS */
#endif /* !__DOXYGEN__ */
