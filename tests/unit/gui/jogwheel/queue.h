#pragma once
//fake rtos queue for tests
#include <stdint.h>
#include <queue>

using QueueHandle_t = void *;
using BaseType_t = int;
enum { pdFALSE = 0,
    pdTRUE = 1,
    pdFAIL = pdFALSE,

    pdPASS = pdTRUE };

inline void *GetQ32() {
    static std::queue<uint32_t> ret;
    return &ret;
}

inline void *GetQ8() {
    static std::queue<uint8_t> ret;
    return &ret;
}

inline QueueHandle_t xQueueCreate(uint32_t uxQueueLength, uint32_t uxItemSize) {
    if (uxItemSize == 1) {
        return GetQ8();
    } else {
        return GetQ32();
    }
}

template <class T>
inline int xQueueReceiveTest(std::queue<T> *queue, void *const pvBuffer) {
    if (queue->empty())
        return pdFAIL;
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
    if (xQueue == GetQ8()) {
        return xQueueReceiveTest((std::queue<uint8_t> *)xQueue, pvBuffer);
    } else {
        return xQueueReceiveTest((std::queue<uint32_t> *)xQueue, pvBuffer);
    }
}

inline BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *const pvItemToQueue, BaseType_t *const pxHigherPriorityTaskWoken) {
    if (xQueue == GetQ8()) {
        return xQueueSendFromISR_Test((std::queue<uint8_t> *)xQueue, pvItemToQueue);
    } else {
        return xQueueSendFromISR_Test((std::queue<uint32_t> *)xQueue, pvItemToQueue);
    }
}
