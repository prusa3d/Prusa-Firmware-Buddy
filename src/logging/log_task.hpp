#pragma once

#include <atomic>
#include <common/freertos_binary_semaphore.hpp>
#include <common/freertos_queue.hpp>
#include <logging/log.h>

#include <FreeRTOS.h>
#include <task.h>

class LogTask {
private:
    struct QueueItem {
        log_event_t *event;
        freertos::BinarySemaphore *semaphore;
    };
    freertos::Queue<QueueItem, 4> queue;
    static constexpr size_t STACK_SIZE = 320;
    std::atomic<TaskHandle_t> task_handle;
    StaticTask_t task_buffer;
    StackType_t task_stack[STACK_SIZE];

    void run();

public:
    /// Start new FreeRTOS task responsible for processing logging queue.
    LogTask();

    /// Put event into logging queue and block until it is processed.
    void send(log_event_t *);
};
