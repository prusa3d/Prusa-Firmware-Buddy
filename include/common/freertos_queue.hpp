#pragma once
#include "FreeRTOS.h"
#include "queue.h"

namespace freertos {

template <class T, size_t N>
struct Queue {
    Queue() {
        auto queue = xQueueCreateStatic(N, sizeof(T), storage, &static_queue);
        // We are creating static FreeRTOS object here, supplying our own buffer
        // to be used by FreeRTOS. FreeRTOS constructs an object in that memory
        // and gives back a handle, which in current version is just a pointer
        // to the same buffer we provided. If this ever changes, we will have to
        // store the handle separately, but right now we can just use the pointer
        // to the buffer instead of the handle and save 4 bytes per instance.
        configASSERT(queue == this->queue());
    }
    bool send(const T &payload, TickType_t ticks_to_wait = portMAX_DELAY) {
        return xQueueSend(queue(), &payload, ticks_to_wait) == pdTRUE;
    }
    bool receive(T &payload, TickType_t ticks_to_wait = portMAX_DELAY) {
        return xQueueReceive(queue(), &payload, ticks_to_wait) == pdTRUE;
    }

private:
    QueueHandle_t queue() { return reinterpret_cast<QueueHandle_t>(&static_queue); };
    StaticQueue_t static_queue;
    uint8_t storage[N * sizeof(T)];
};

} // namespace freertos
