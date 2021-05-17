/**
 * \file            esp_sys_freertos_os.c
 * \brief           System dependant functions
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
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Author:          Adrian Carpenter (FreeRTOS port)
 */

#include "system/esp_sys.h"
#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#if !__DOXYGEN__
/* Mutex ID for main protection */
static SemaphoreHandle_t sys_mutex;

typedef struct freertos_mbox {
    void *d;
} freertos_mbox;

uint8_t
esp_sys_init(void) {
    sys_mutex = xSemaphoreCreateMutex();
    return sys_mutex == NULL ? 0 : 1;
}

uint32_t
esp_sys_now(void) {
    return xTaskGetTickCount();
}

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
    *p = xSemaphoreCreateRecursiveMutex();
    return *p != NULL;
}

uint8_t
esp_sys_mutex_delete(esp_sys_mutex_t* p) {
    vSemaphoreDelete(*p);
    return 1;
}

uint8_t
esp_sys_mutex_lock(esp_sys_mutex_t* p) {
    return xSemaphoreTakeRecursive(*p, portMAX_DELAY) == pdPASS;
}

uint8_t
esp_sys_mutex_unlock(esp_sys_mutex_t* p) {
    return xSemaphoreGiveRecursive(*p) == pdPASS;
}

uint8_t
esp_sys_mutex_isvalid(esp_sys_mutex_t* p) {
    return *p != NULL;
}

uint8_t
esp_sys_mutex_invalid(esp_sys_mutex_t* p) {
    *p = ESP_SYS_MUTEX_NULL;
    return 1;
}

uint8_t
esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt) {
    *p = xSemaphoreCreateBinary();

    if (*p != NULL && !cnt) {
        xSemaphoreTake(*p, 0);
    }
    return *p != NULL;
}

uint8_t
esp_sys_sem_delete(esp_sys_sem_t* p) {
    vSemaphoreDelete(*p);
    return 1;
}

uint32_t
esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout) {
    uint32_t t = xTaskGetTickCount();
    return xSemaphoreTake(*p, timeout == 0 ? portMAX_DELAY : timeout) == pdPASS ? (xTaskGetTickCount() - t) : ESP_SYS_TIMEOUT;
}

uint8_t
esp_sys_sem_release(esp_sys_sem_t* p) {
    return xSemaphoreGive(*p) == pdPASS;
}

uint8_t
esp_sys_sem_isvalid(esp_sys_sem_t* p) {
    return *p != NULL;
}

uint8_t
esp_sys_sem_invalid(esp_sys_sem_t* p) {
    *p = ESP_SYS_SEM_NULL;
    return 1;
}

uint8_t
esp_sys_mbox_create(esp_sys_mbox_t* b, size_t size) {
    *b = xQueueCreate(size, sizeof(freertos_mbox));
    return *b != NULL;
}

uint8_t
esp_sys_mbox_delete(esp_sys_mbox_t* b) {
    if (uxQueueMessagesWaiting(*b)) {
        return 0;
    }
    vQueueDelete(*b);
    return 1;
}

uint32_t
esp_sys_mbox_put(esp_sys_mbox_t* b, void* m) {
    freertos_mbox mb;
    uint32_t t = xTaskGetTickCount();

    mb.d = m;
    xQueueSend(*b, &mb, portMAX_DELAY);
    return xTaskGetTickCount()-t;
}

uint32_t
esp_sys_mbox_get(esp_sys_mbox_t* b, void** m, uint32_t timeout) {
    freertos_mbox mb;
    uint32_t t = xTaskGetTickCount();

    if (xQueueReceive(*b, &mb, timeout == 0 ? portMAX_DELAY : timeout)) {
       *m = mb.d;
       return xTaskGetTickCount()-t;
    }
    return ESP_SYS_TIMEOUT;
}

uint8_t
esp_sys_mbox_putnow(esp_sys_mbox_t* b, void* m) {
    freertos_mbox mb;

    mb.d = m;
    return xQueueSendFromISR(*b, &mb, 0) == pdPASS;
}

uint8_t
esp_sys_mbox_getnow(esp_sys_mbox_t* b, void** m) {
    freertos_mbox mb;

    if (xQueueReceive(*b, &mb, 0)) {
       *m = mb.d;
       return 1;
    }
    return 0;
}

uint8_t
esp_sys_mbox_isvalid(esp_sys_mbox_t* b) {
    return *b != NULL;
}

uint8_t
esp_sys_mbox_invalid(esp_sys_mbox_t* b) {
    *b = ESP_SYS_MBOX_NULL;
    return 1;
}

uint8_t
esp_sys_thread_create(esp_sys_thread_t* t, const char* name, esp_sys_thread_fn thread_func, void* const arg, size_t stack_size, esp_sys_thread_prio_t prio) {
    return xTaskCreate(thread_func, name, stack_size, arg, prio, t) == pdPASS ? 1 : 0;
}

uint8_t
esp_sys_thread_terminate(esp_sys_thread_t* t) {
    vTaskDelete(*t);
    return 1;
}

uint8_t
esp_sys_thread_yield(void) {
    taskYIELD();
    return 1;
}

#endif /* !__DOXYGEN__ */
