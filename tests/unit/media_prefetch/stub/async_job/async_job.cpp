#include "async_job.hpp"

void AsyncJob::issue(const std::function<void(AsyncJobExecutionControl &)> &f) {
    AsyncJobExecutionControl ctrl;
    ctrl.job = this;
    was_discarded_ = false;
    f(ctrl);
    discard_after = {};
}

bool AsyncJobExecutionControl::is_discarded() {
    job->discard_check_count++;

    if (job->was_discarded_ || (job->discard_after && --(*job->discard_after) <= 0)) {
        job->was_discarded_ = true;
        return true;
    }

    return false;
}
