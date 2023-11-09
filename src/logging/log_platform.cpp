#include "log.h"
#include "cmsis_os.h"
#include "task.h"
#include "timing.h"

log_timestamp_t log_platform_timestamp_get() {
    auto timestamp = get_timestamp();
    return { timestamp.sec, timestamp.us };
}

log_task_id_t log_platform_task_id_get() {
    if (xPortIsInsideInterrupt()) {
        return -1;
    }
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return -2;
    }
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    return uxTaskGetTaskNumber(task_handle);
}

bool log_platform_is_low_on_resources() {
    if (xPortIsInsideInterrupt()) {
        return true;
    }
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return true;
    }
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    int offset = sizeof(StackType_t) + 2 * sizeof(ListItem_t) + sizeof(UBaseType_t);
    uint8_t *bottom_of_stack = (uint8_t *)((uint32_t *)((uint8_t *)task_handle + offset))[0];
    uint8_t *current_stack_pos = (uint8_t *)&offset;
    int available_bytes = current_stack_pos - bottom_of_stack;
    return available_bytes < 512;
}
