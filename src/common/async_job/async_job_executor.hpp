#pragma once

#include <common/freertos_mutex.hpp>
#include <common/freertos_binary_semaphore.hpp>

class AsyncJob;

class AsyncJobExecutor final {
    friend class AsyncJob;
    friend class AsyncJobExecutionControl;

public:
    AsyncJobExecutor();
    ~AsyncJobExecutor();

    /// Returns default instance of the executor
    static AsyncJobExecutor &default_instance();

private:
    /// Routine that runs on the worker task
    void thread_routine();

private:
    /// Fields that should only be accessed with locked \p mutex
    struct {
        /// First job in the linked list (the one to be executed)
        AsyncJob *first_job = nullptr;

        /// Last job in the queue
        AsyncJob *last_job = nullptr;

        /// Marks that the AsyncJob instance for the currently running job was destroyed/discarded
        bool current_job_discarded = false;

    } synchronized_data;

    freertos::Mutex mutex;

    /// Semaphore used for waiting for having at least one job in the queue
    freertos::BinarySemaphore nonempty_queue_semaphore;

private:
    osStaticThreadDef_t thread_def;
    osThreadId thread_id;
    std::array<uint32_t, 1024> thread_stack;
};
