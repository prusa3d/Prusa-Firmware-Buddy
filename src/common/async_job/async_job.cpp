#include "async_job.hpp"

AsyncJobBase::~AsyncJobBase() {
    discard();
}

bool AsyncJobBase::is_active() const {
    const auto state = state_.load();

    // The job instance can possibly be changed by some other thread only if it is in the queue or if it is currently being executed.
    // In all other cases, the owner of the AsyncJob instance is the only one who accesses the instance.

    // !!! For this to work correctly, the state must be changed to finished/cancelled as the very last thing before unlocking the mutex on the worker thread
    return (state == State::queued) || (state == State::running);
}

void AsyncJobBase::issue(const Callback &callback, AsyncJobExecutor &executor) {
    // Discard previous job this instance was holding
    discard();

    std::lock_guard mutex_guard(executor.mutex);

    // Insert the job in the linked list
    {
        assert(next_job == nullptr);
        assert(previous_job == nullptr);

        auto &ex = executor.synchronized_data;

        if (ex.first_job != nullptr) {
            // The linked list is not empty -> insert this after the last job
            assert(ex.last_job != nullptr);
            previous_job = ex.last_job;
            previous_job->next_job = this;

        } else {
            // The linked list was empty -> make this a first job and wake the executor
            assert(!ex.first_job);
            ex.first_job = this;
            executor.empty_queue_condition.notify_one();
        }

        ex.last_job = this;
    }

    this->executor = &executor;
    this->callback = callback;
    state_ = State::queued;
}

bool AsyncJobBase::try_cancel() {
    // If the task is not active, we don't have to synchronize and can just straight up fail.
    if (!is_active()) {
        return false;
    }

    assert(executor);
    std::lock_guard mutex_guard(executor->mutex);

    if (state_ != State::queued) {
        return false;
    }

    unqueue_nolock();
    state_ = State::cancelled;
    return true;
}

void AsyncJobBase::discard() {
    // If the task is active, it can be potentially modified by the executor, and thus we need to synchronize the changes
    if (is_active()) {
        assert(executor);
        std::lock_guard mutex_guard(executor->mutex);

        switch (state_.load()) {

        case State::queued:
            // The job is queued -> remove it from the queue
            unqueue_nolock();
            break;

        case State::running:
            // The job is currently being processed -> mark it as discarded so that the worker task does not try to write to this structure afterwards
            executor->synchronized_data.current_job_discarded = true;
            break;

        case State::finished:
            // The job is finished -> no need to do anything
            break;

        default:
            // All other cases should have been handled in the early check before locking the mutex
            assert(false);
            break;
        }
    }

    state_ = State::idle;
    callback = {};
    executor = {};
}

void AsyncJobBase::unqueue_nolock() {
    assert(state_ == State::queued);

    auto &ex = executor->synchronized_data;

    if (previous_job) {
        previous_job->next_job = next_job;
        previous_job = nullptr;
    } else {
        assert(ex.first_job == this);
        ex.first_job = next_job;
    }

    if (next_job) {
        next_job->previous_job = previous_job;
        next_job = nullptr;
    } else {
        assert(ex.last_job == this);
        ex.last_job = previous_job;
    }
}
