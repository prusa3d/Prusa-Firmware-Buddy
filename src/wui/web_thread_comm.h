#ifndef WEB_THREAD_COMM_H
#define WEB_THREAD_COMM_H

#include "cmsis_os.h"

extern osThreadId web_task; // task handle
extern osMessageQId web_queue; // input queue (uint8_t)
extern osSemaphoreId web_sema; // semaphore handle

typedef struct {

    uint16_t flags;
    uint64_t value;

} web_thread_communication_t;

typedef struct {
    char time_elapsed[13];
    char time_remain[9];
    char file_name[95];
} shared_mem_t;

extern shared_mem_t * web_shared_memory;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t web_thread_comm_init();

void web_thread_comm_loop();

#ifdef __cplusplus
}
#endif

#endif //WEB_THREAD_COMM_H
