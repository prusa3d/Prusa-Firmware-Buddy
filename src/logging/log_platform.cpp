#include "log.h"

#include "timing.h"

#include <FreeRTOS.h> // must appear in source files before include task.h
#include <task.h>

log_timestamp_t log_platform_timestamp_get() {
    auto timestamp = get_timestamp();
    return { timestamp.sec, timestamp.us };
}

log_task_id_t log_platform_task_id_get() {
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    return uxTaskGetTaskNumber(task_handle);
}
