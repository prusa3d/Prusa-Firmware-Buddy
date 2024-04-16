#pragma once

#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    SemaphoreHandle_t semaphore;
    SemaphoreHandle_t mutex;
    uint8_t max_readers;

    StaticSemaphore_t semaphore_data;
    StaticSemaphore_t mutex_data;
} RWMutex_t;

void rw_mutex_init(RWMutex_t *rw_mutex, uint8_t max_readers);
void rw_mutex_reader_take(RWMutex_t *rw_mutex);
bool rw_mutex_reader_try_take(RWMutex_t *rw_mutex);
void rw_mutex_reader_give(RWMutex_t *rw_mutex);
void rw_mutex_writer_take(RWMutex_t *rw_mutex);
bool rw_mutex_writer_try_take(RWMutex_t *rw_mutex);
void rw_mutex_writer_give(RWMutex_t *rw_mutex);

#ifdef __cplusplus
}

#endif
