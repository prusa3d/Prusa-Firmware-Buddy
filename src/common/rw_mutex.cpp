#include "rw_mutex.h"

void rw_mutex_init(RWMutex_t *rw_mutex, uint8_t max_readers) {
    rw_mutex->semaphore = xSemaphoreCreateCountingStatic(max_readers, max_readers, &rw_mutex->semaphore_data);
    rw_mutex->mutex = xSemaphoreCreateMutexStatic(&rw_mutex->mutex_data);
    rw_mutex->max_readers = max_readers;
}

void rw_mutex_reader_take(RWMutex_t *rw_mutex) {
    xSemaphoreTake(rw_mutex->semaphore, portMAX_DELAY);
}

bool rw_mutex_reader_try_take(RWMutex_t *rw_mutex) {
    return xSemaphoreTake(rw_mutex->semaphore, 0) == pdTRUE;
}

void rw_mutex_reader_give(RWMutex_t *rw_mutex) {
    xSemaphoreGive(rw_mutex->semaphore);
}

void rw_mutex_writer_take(RWMutex_t *rw_mutex) {
    xSemaphoreTake(rw_mutex->mutex, portMAX_DELAY);
    for (uint_fast8_t count = rw_mutex->max_readers; count; --count) {
        xSemaphoreTake(rw_mutex->semaphore, portMAX_DELAY);
    }
}

bool rw_mutex_writer_try_take(RWMutex_t *rw_mutex) {
    if (xSemaphoreTake(rw_mutex->mutex, 0) == pdFALSE) {
        return false;
    }
    uint_fast8_t count = 0;
    for (; count < rw_mutex->max_readers; count++) {
        if (xSemaphoreTake(rw_mutex->semaphore, 0) == pdFALSE) {
            break;
        }
    }
    if (count == rw_mutex->max_readers) {
        return true;
    }
    for (; count > 0; count--) {
        xSemaphoreGive(rw_mutex->semaphore);
    }
    xSemaphoreGive(rw_mutex->mutex);
    return false;
}

void rw_mutex_writer_give(RWMutex_t *rw_mutex) {
    for (uint_fast8_t count = rw_mutex->max_readers; count; --count) {
        xSemaphoreGive(rw_mutex->semaphore);
    }
    xSemaphoreGive(rw_mutex->mutex);
}
