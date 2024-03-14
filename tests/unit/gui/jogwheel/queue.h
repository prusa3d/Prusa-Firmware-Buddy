#pragma once
// fake rtos queue for tests
#include <stdint.h>
#include <queue>

using QueueHandle_t = void *;
using BaseType_t = int;
enum { pdFALSE = 0,
    pdTRUE = 1,
    pdFAIL = pdFALSE,

    pdPASS = pdTRUE };

// not general version, queue with 4B items only
inline QueueHandle_t xQueueCreate(uint32_t uxQueueLength, uint32_t uxItemSize) {
    static std::queue<uint32_t> q32;
    std::queue<uint32_t> empty;
    std::swap(q32, empty);
    return &q32;
}

template <class T>
inline int xQueueReceiveTest(std::queue<T> *queue, void *const pvBuffer) {
    if (queue->empty()) {
        return pdFAIL;
    }
    T *const pT = (T *const)pvBuffer;
    *pT = queue->front();
    queue->pop();
    return pdPASS;
}

template <class T>
inline BaseType_t xQueueSendFromISR_Test(std::queue<T> *queue, const void *const pvItemToQueue) {
    const T *const pT = (const T *const)pvItemToQueue;
    queue->push(*pT);
    return pdPASS;
}

inline int xQueueReceive(QueueHandle_t xQueue, void *const pvBuffer, uint32_t xTicksToWait) {
    return xQueueReceiveTest((std::queue<uint32_t> *)xQueue, pvBuffer);
}

inline BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *const pvItemToQueue, BaseType_t *const pxHigherPriorityTaskWoken) {
    return xQueueSendFromISR_Test((std::queue<uint32_t> *)xQueue, pvItemToQueue);
}
