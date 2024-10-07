#include <freertos/queue.hpp>

#include <cstdlib>

// FreeRTOS.h must be included before queue.h
#include <FreeRTOS.h>
#include <queue.h>

namespace freertos {

static QueueHandle_t handle_cast(QueueBase::Storage &queue_storage) {
    return static_cast<QueueHandle_t>(static_cast<void *>(queue_storage.data()));
}

QueueBase::QueueBase(size_t item_count, size_t item_size, uint8_t *item_storage) {
    // If these asserts start failing, go fix the constants.
    static_assert(queue_storage_size == sizeof(StaticQueue_t));
    static_assert(queue_storage_align == alignof(StaticQueue_t));

    QueueHandle_t queue = xQueueCreateStatic(item_count, item_size, item_storage, reinterpret_cast<StaticQueue_t *>(&queue_storage));
    // We are creating static FreeRTOS object here, supplying our own buffer
    // to be used by FreeRTOS. FreeRTOS constructs an object in that memory
    // and gives back a handle, which in current version is just a pointer
    // to the same buffer we provided. If this ever changes, we will have to
    // store the handle separately, but right now we can just use the pointer
    // to the buffer instead of the handle and save 4 bytes per instance.
    configASSERT(queue == handle_cast(queue_storage));
}

QueueBase::~QueueBase() {
    vQueueDelete(handle_cast(queue_storage));
}

void QueueBase::send(const void *payload) {
    if (xQueueSend(handle_cast(queue_storage), payload, portMAX_DELAY) != pdTRUE) {
        static_assert(INCLUDE_vTaskSuspend);
        // Since we are waiting forever and have task suspension, this should never happen.
        std::abort();
    }
}

bool QueueBase::send_from_isr(const void *payload) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(handle_cast(queue_storage), payload, &higher_priority_task_woken);
    return higher_priority_task_woken;
}

void QueueBase::receive(void *payload) {
    if (xQueueReceive(handle_cast(queue_storage), payload, portMAX_DELAY) != pdTRUE) {
        static_assert(INCLUDE_vTaskSuspend);
        // Since we are waiting forever and have task suspension, this should never happen.
        std::abort();
    }
}

bool QueueBase::try_send(const void *payload, size_t milliseconds_to_wait) {
    return xQueueSend(handle_cast(queue_storage), payload, pdMS_TO_TICKS(milliseconds_to_wait)) == pdTRUE;
}

bool QueueBase::try_receive(void *payload, size_t milliseconds_to_wait) {
    return xQueueReceive(handle_cast(queue_storage), payload, pdMS_TO_TICKS(milliseconds_to_wait)) == pdTRUE;
}

} // namespace freertos
