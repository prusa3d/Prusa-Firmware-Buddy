// Unittest stub

#pragma once

#include <functional>
#include <optional>

class AsyncJob;

class AsyncJobExecutionControl {
public:
    AsyncJob *job = nullptr;

    bool is_discarded();
};

class AsyncJobExecutor {

public:
    static constexpr int worker_count() {
        return 1;
    }
};

class AsyncJob {

public:
    bool is_active() {
        return false;
    }

    void discard() {
    }

    void issue(const std::function<void(AsyncJobExecutionControl &)> &f);

    bool was_discarded() {
        return was_discarded_;
    }

    // If set, marks the job as discarded after X checks
    std::optional<int> discard_after;

    bool was_discarded_ = false;

    size_t discard_check_count = 0;
};
