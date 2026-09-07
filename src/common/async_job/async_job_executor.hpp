/// @file
#pragma once

#include <array>
#include <cmsis_os.h>
#include <cstdint>
#include <freertos/mutex.hpp>
#include <freertos/wait_condition.hpp>

class AsyncJobBase;

class AsyncJobExecutor final {
    friend class AsyncJobBase;
    friend class AsyncJobExecutionControl;

public:
    AsyncJobExecutor();
    ~AsyncJobExecutor();

    /// Returns default instance of the executor
    static AsyncJobExecutor &default_instance();

public:
    /// \returns number of worker thread this executor manages
    static inline constexpr int worker_count() {
        return 1;
    }

private:
    /// Routine that runs on the worker task
    void thread_routine();

private:
    /// Fields that should only be accessed with locked \p mutex
    struct {
        /// First job in the linked list (the one to be executed)
        AsyncJobBase *first_job = nullptr;

        /// Last job in the queue
        AsyncJobBase *last_job = nullptr;

        /// Job that is currently being executed on the thread
        /// Discarding the job is indicated by setting this to nullptr
        AsyncJobBase *current_job = nullptr;

    } synchronized_data;

    freertos::Mutex mutex;

    /// Wait condition for the executor, when the queue is empty
    freertos::WaitCondition empty_queue_condition;

private:
    osStaticThreadDef_t thread_def;
    osThreadId thread_id;
    std::array<uint32_t, 1024> thread_stack;
};
