#include "async_job_executor.hpp"

#include "async_job.hpp"
#include <priorities_config.h>

static __attribute__((section(".ccmram"))) AsyncJobExecutor default_instance;

AsyncJobExecutor::AsyncJobExecutor() {
    static constexpr auto thread_func = +[](const void *param) {
        reinterpret_cast<AsyncJobExecutor *>(const_cast<void *>(param))->thread_routine();
    };
    osThreadStaticDef(worker_thread, thread_func, TASK_PRIORITY_ASYNC_JOB_EXECUTOR, 0, thread_stack.size(), thread_stack.data(), &thread_def);
    thread_id = osThreadCreate(osThread(worker_thread), this);
}

AsyncJobExecutor::~AsyncJobExecutor() {
    // Not necessary to implement it, so not doing it for now.
    // If this is necessary, the following thins need to be considered:
    // * Assert that there are no tasks in the queue
    //   (the executor owns the mutex, so if there were any tasks in the queue possibly trying to lock to a deleted lock, things would go haywire)
    // * Signal the thread to quit, wait for it to finish, clean up
    std::terminate();
}

AsyncJobExecutor &AsyncJobExecutor::default_instance() {
    return ::default_instance;
}

void AsyncJobExecutor::thread_routine() {
    using State = AsyncJobBase::State;
    using Callback = AsyncJobBase::Callback;

    while (true) {
        Callback callback;

        // This pointer is unsafe to access outside mutex locked areas
        AsyncJobBase *job;

        // Pop a job from the list and obtain the callback
        {
            std::unique_lock mutex_guard(mutex);

            while (!synchronized_data.first_job) {
                empty_queue_condition.wait(mutex_guard);
            }

            job = synchronized_data.first_job;
            assert(job);
            assert(job->state_ == State::queued);

            synchronized_data.current_job_discarded = false;
            callback = job->callback;

            job->unqueue_nolock();
            job->state_ = State::running;
        }

        // Execute the callback
        {
            AsyncJobExecutionControl ctrl(*this);
            callback(ctrl);
        }

        // Report the job to be done
        {
            std::lock_guard mutex_guard(mutex);

            // The job was discarded -> do not update the AsyncJob instance
            if (synchronized_data.current_job_discarded) {
                continue;
            }

            assert(job->state_ == State::running);

            // !!! This must be done as the last thing before unlocking the mutex to make AsyncJob::is_active() work correctly
            job->state_ = State::finished;
        }
    }
}
