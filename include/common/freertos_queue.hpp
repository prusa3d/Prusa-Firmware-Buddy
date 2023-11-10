#pragma once
#include "FreeRTOS.h"
#include "queue.h"

namespace freertos {

template <class T, size_t N>
struct Queue {
    Queue()
        : queue(xQueueCreateStatic(N, sizeof(T), storage, &static_queue)) {}
    bool send(const T &payload, TickType_t ticks_to_wait = portMAX_DELAY) {
        return xQueueSend(queue, &payload, ticks_to_wait) == pdTRUE;
    }
    bool receive(T &payload, TickType_t ticks_to_wait = portMAX_DELAY) {
        return xQueueReceive(queue, &payload, ticks_to_wait) == pdTRUE;
    }

private:
    QueueHandle_t queue;
    StaticQueue_t static_queue;
    uint8_t storage[N * sizeof(T)];
};

} // namespace freertos
