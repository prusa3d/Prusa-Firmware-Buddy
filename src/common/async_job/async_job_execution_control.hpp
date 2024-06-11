#pragma once

#include <functional>

class AsyncJobExecutor;

/// Helper class that is passed to the currently executed AsyncJob callback.
/// The class provides some functions that allow the job to reporting/checking back with the caller
class AsyncJobExecutionControl {
    friend class AsyncJobExecutor;

public:
    /// This function is intended as a safe way to return results from the callback function to the issuer.
    /// Calls function \p f in a synchronized manner with the currently running task (visitor pattern).
    /// The \p f should be as fast as possible - only to store the results somewhere.
    /// The function is called with the synchronization mutex locked and only in the case that the task was not cancelled/discarded.
    /// \returns true if the function was executed
    bool with_synchronized(const std::function<void()> &f);

    /// \returns true if the job was discarded.
    /// This has only informational character and is intended to be used only as an option to early exit tasks that are no longer required.
    bool is_discarded();

private:
    AsyncJobExecutionControl(AsyncJobExecutor &executor)
        : executor(executor) {}

    ~AsyncJobExecutionControl() = default;

private:
    AsyncJobExecutor &executor;
};
