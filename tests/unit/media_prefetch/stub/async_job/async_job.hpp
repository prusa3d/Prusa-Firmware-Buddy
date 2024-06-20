// Unittest stub

#pragma once

#include <functional>

class AsyncJobExecutionControl {
public:
    bool is_discarded() {
        return false;
    }
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

    void issue(const std::function<void(AsyncJobExecutionControl &)> &f) {
        AsyncJobExecutionControl ctrl;
        f(ctrl);
    }
};
