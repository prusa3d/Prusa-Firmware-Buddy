#include <freertos/binary_semaphore.hpp>

#include <cstdlib>

#include <bsod/bsod.h>

// FreeRTOS.h must be included before semphr.h
#include <FreeRTOS.h>
#include <semphr.h>

#include "ticks.hpp"

namespace freertos {

BinarySemaphore::BinarySemaphore() {
    // If these asserts start failing, go fix the constants.
    static_assert(semaphore_storage_size == sizeof(StaticSemaphore_t));
    static_assert(semaphore_storage_align == alignof(StaticSemaphore_t));

    handle = xSemaphoreCreateBinaryStatic(reinterpret_cast<StaticSemaphore_t *>(&semaphore_storage));
}

BinarySemaphore::~BinarySemaphore() {
    vSemaphoreDelete(SemaphoreHandle_t(handle));
}

void BinarySemaphore::release() {
    if (xSemaphoreGive(SemaphoreHandle_t(handle)) != pdTRUE) {
        // Since the semaphore was obtained correctly, this should never happen.
        std::abort();
    }
}

void BinarySemaphore::release_from_isr() {
    // A permit already waiting means someone released without a matching acquire
    release_assert(try_release_from_isr());
}

bool BinarySemaphore::try_release_from_isr() {
    BaseType_t higher_priority_task_woken = pdFALSE;
    // errQUEUE_FULL is the only other outcome, and it means the wakeup is already pending
    const bool released = xSemaphoreGiveFromISR(SemaphoreHandle_t(handle), &higher_priority_task_woken) == pdTRUE;
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return released;
}

void BinarySemaphore::release_blocking() {
    static_assert(INCLUDE_vTaskSuspend);

    // Same as xSemaphoreGive macro, just the ticksToWait is portMAX_DELAY
    while (xQueueGenericSend(SemaphoreHandle_t(handle), NULL, portMAX_DELAY, queueSEND_TO_BACK) != pdTRUE) {
        // xTaskAbortDelay was called on this task, retry
    }
}

void BinarySemaphore::acquire() {
    static_assert(INCLUDE_vTaskSuspend);

    while (xSemaphoreTake(SemaphoreHandle_t(handle), portMAX_DELAY) != pdTRUE) {
        // xTaskAbortDelay was called on this task, retry
    }
}

bool BinarySemaphore::try_acquire_for(uint32_t ms) {
    return xSemaphoreTake(SemaphoreHandle_t(handle), ms_to_ticks(ms)) == pdTRUE;
}

} // namespace freertos
