#include "log_task.hpp"

#include <cstdlib>

LogTask::LogTask()
    : task_handle { nullptr } {
    TaskHandle_t local_task_handle = xTaskCreateStatic(
        +[](void *ctx) { static_cast<LogTask *>(ctx)->run(); },
        "log_task",
        STACK_SIZE,
        this,
        tskIDLE_PRIORITY + 2,
        task_stack,
        &task_buffer);
    if (local_task_handle == nullptr) {
        abort(); // Since we are allocating statically, this should never happen.
    }
}

void LogTask::run() {
    task_handle.store(xTaskGetCurrentTaskHandle());
    for (;;) {
        QueueItem item;
        queue.receive(item);
        log_task_process_event(item.event);
        item.semaphore->release();
    }
}

void LogTask::send(log_event_t *event) {
    if (xPortIsInsideInterrupt()) {
        // Note: We do not support logging from the interrupt handler.
        //       It is usually a bad idea anyway.
        // FIXME: But perhaps we can at least log via RTT.
        return;
    }
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        // Note: We require FreeRTOS scheduler to be running in order to drain
        //       the logging queue. If the scheduler was not yet started
        //       (i.e. shortly after boot) we discard the message, otherwise
        //       we would block on semaphore and never wake up.
        // FIXME: But perhaps we can at least log via RTT in such case.
        return;
    }
    if (xTaskGetCurrentTaskHandle() == task_handle) {
        // FIXME: For now, we do not support logging from the logging thread.
        //        In order to support that, we would have to support non-blocking
        //        insertion to the queue, which would in turn require solving the
        //        lifetime issues. We would also need to disable the semaphore.
        return;
    }
    if (task_handle == nullptr) {
        // Note: This fixes rare race condition when some task is trying to log
        //       before the logging task is up. In order to prevent deadlock,
        //       we throw away the message.
        return;
    }

    freertos::BinarySemaphore semaphore;
    QueueItem item {
        .event = event,
        .semaphore = &semaphore,
    };
    // Block until we succesfully send the message. Should happen only when
    // too many tasks are trying to log.
    queue.send(item);
    // Block until logging task wakes us. We need this to keep the pointers
    // inside queue item alive.
    semaphore.acquire();
}
