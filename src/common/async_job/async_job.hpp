#pragma once

#include <functional>
#include <atomic>

#include "async_job_executor.hpp"
#include "async_job_execution_control.hpp"

/// Represents a job that is to be asynchornously executed in an executor in a queue-like manner.
/// In order for the job to execute, the instance of AsyncJob should not be destructed.
/// Destroying the AsyncJob while the job is not yet done is valid, but it can result in the job not being executed.
class AsyncJobBase {
    friend class AsyncJobExecutor;

public:
    using Callback = std::function<void(AsyncJobExecutionControl &control)>;
    using Executor = AsyncJobExecutor;

    enum class State {
        /// Not queued
        idle,

        /// The job is queued to be executed
        /// This is the only state where \p next_job and \p previous_job are not null
        queued,

        /// The job is currently running on the worker thread
        running,

        /// The job has been successfully executed
        finished,

        /// The job has been successfully cancelled
        cancelled,
    };

public:
    AsyncJobBase() = default;

    /// Destroying the AsyncJob while the job is not yet done is valid, but it can result in the job not being executed.
    ~AsyncJobBase();

public:
    /// Current state of the job.
    /// The state can change from queued to running to finished asynchronously at any time, so keep that in mind.
    State state() const {
        return state_.load();
    }

    /// \returns if the task is either queued or running currently.
    /// If this function returns false, it is safe to assume that the job will not be modified asynchronously from the executor.
    bool is_active() const;

    /// Attemps to cancel the job. May not be successfull if the job has already running or done.
    /// \returns true if the cancellation was successfull (if the job changed from queued to cancelled state)
    bool try_cancel();

    /// Similar to cancel, but completely disconnects the job state from the current instance so it can no longer be reported about and such.
    /// Also changes the state to \p idle.
    void discard();

protected:
    /// Enqueues the callback to be executed on a worker task.
    /// If the AsyncJob already contains a running task, discards it.
    /// !!! The callback execution can live longer than the lifespan of the issuer AsyncJob instance. !!!
    /// !!! For returning results from the callback, use \p with_current_job_synchronized() !!!
    void issue(const Callback &callback, Executor &executor = Executor::default_instance());

private:
    /// Unlinks this instance from the linked list.
    /// !!! Assumes the mutex is locked. !!!
    void unqueue_nolock();

private:
    /// Function to be executed in the worker task
    Callback callback;

    Executor *executor = nullptr;

    // The handles form a double linked list. This way, we don't have any fixed-sized queue that could overflow.
    // The linked list is secured by a mutex (defined in the cpp file).
    AsyncJobBase *next_job = nullptr;

    AsyncJobBase *previous_job = nullptr;

    /// Protected by mutex for writing, but still atomic for reading
    std::atomic<State> state_ = State::idle;
};

class AsyncJob final : public AsyncJobBase {

public:
    // Make issue public, this is as basic as it can get
    using AsyncJobBase::issue;
};

/// AsyncJob subclass with interface for returning a value
template <typename Result_>
class AsyncJobWithResult final : public AsyncJobBase {

public:
    using Result = Result_;
    using Callback = std::function<void(AsyncJobExecutionControl &control, Result &result)>;

public:
    inline const Result &result() const {
        assert(state() == State::finished);
        return result_;
    }

    void issue(const Callback &callback, Executor &executor = Executor::default_instance()) {
        // We cannot capture this in the lambda, because stdext::inplace_function wouldn't fit into stdext::inplace_function
        callback_with_result_ = callback;

        AsyncJobBase::issue([this](AsyncJobExecutionControl &control) {
            Callback callback;

            // Obtain the actual callback in a safe manner - we need to make sure the job handle exists while obtaining
            if (!control.with_synchronized([&] { callback = this->callback_with_result_; })) {
                return;
            }

            // Execute the actual function
            Result result;
            callback(control, result);

            // Store the result, again in a safe manner
            control.with_synchronized([&] {
                this->result_ = result;
            });
        },
            executor);
    }

private:
    Callback callback_with_result_;
    Result result_;
};
