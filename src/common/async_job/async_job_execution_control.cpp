#include "async_job_execution_control.hpp"

#include "async_job_executor.hpp"

bool AsyncJobExecutionControl::with_synchronized(const std::function<void()> &f) {
    std::lock_guard mutex_guard(executor.mutex);

    if (executor.synchronized_data.current_job_discarded) {
        return false;
    }

    f();
    return true;
}

bool AsyncJobExecutionControl::is_discarded() {
    // The result is informative, so we don't need to use mutex here.
    return executor.synchronized_data.current_job_discarded;
}
