#include <freertos/queue.hpp>

#include <cstdlib>
#include <type_traits>

// FreeRTOS.h must be included before queue.h
#include <FreeRTOS.h>
#include <queue.h>

namespace freertos {

QueueBase::QueueBase(size_t item_count, size_t item_size, uint8_t *item_storage)
    : queue_storage {}
    , queue_handle {} {
    // If these asserts start failing, go fix the constants.
    static_assert(queue_storage_size == sizeof(StaticQueue_t));
    static_assert(queue_storage_align == alignof(StaticQueue_t));

    static_assert(std::is_same_v<QueueHandle_t, QueueDefinition *>);

    queue_handle = xQueueCreateStatic(item_count, item_size, item_storage, reinterpret_cast<StaticQueue_t *>(&queue_storage));
    configASSERT(queue_handle != nullptr);
}

QueueBase::~QueueBase() {
    vQueueDelete(queue_handle);
}

void QueueBase::send(const void *payload) {
    if (xQueueSend(queue_handle, payload, portMAX_DELAY) != pdTRUE) {
        static_assert(INCLUDE_vTaskSuspend);
        // Since we are waiting forever and have task suspension, this should never happen.
        std::abort();
    }
}

bool QueueBase::send_from_isr(const void *payload) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(queue_handle, payload, &higher_priority_task_woken);
    return higher_priority_task_woken == pdTRUE;
}

void QueueBase::receive(void *payload) {
    if (xQueueReceive(queue_handle, payload, portMAX_DELAY) != pdTRUE) {
        static_assert(INCLUDE_vTaskSuspend);
        // Since we are waiting forever and have task suspension, this should never happen.
        std::abort();
    }
}

bool QueueBase::try_send(const void *payload, size_t milliseconds_to_wait) {
    return xQueueSend(queue_handle, payload, pdMS_TO_TICKS(milliseconds_to_wait)) == pdTRUE;
}

bool QueueBase::try_receive(void *payload, size_t milliseconds_to_wait) {
    return xQueueReceive(queue_handle, payload, pdMS_TO_TICKS(milliseconds_to_wait)) == pdTRUE;
}

} // namespace freertos
