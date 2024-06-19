#pragma once

#include <atomic>
#include <freertos/binary_semaphore.hpp>
#include <freertos/queue.hpp>
#include <logging/log.hpp>

#include <FreeRTOS.h>
#include <task.h>

namespace logging {

class Task {
private:
    struct QueueItem {
        Event *event;
        freertos::BinarySemaphore *semaphore;
    };
    freertos::Queue<QueueItem, 4> queue;
    static constexpr size_t MESSAGE_MAX_SIZE = 128;
    static constexpr size_t STACK_SIZE = MESSAGE_MAX_SIZE + 192;
    std::atomic<TaskHandle_t> task_handle;
    StaticTask_t task_buffer;
    StackType_t task_stack[STACK_SIZE];

    void run();

public:
    /// Start new FreeRTOS task responsible for processing logging queue.
    Task();

    /// Put event into logging queue and block until it is processed.
    void send(Event *);
};

} // namespace logging
