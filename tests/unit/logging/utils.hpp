#pragma once
#include <optional>
#include <functional>
#include <vector>
#include <deque>
#include <string>
#include <logging/log.hpp>

class ScopeGuard {
public:
    template <class Callable>
    ScopeGuard(Callable &&undo_func)
        : f(std::forward<Callable>(undo_func)) {
    }

    ScopeGuard(ScopeGuard &&other)
        : f(std::move(other.f)) {
        other.f = nullptr;
    }

    ~ScopeGuard() {
        if (f) {
            f();
        }
    }

    void dismiss() noexcept {
        f = nullptr;
    }

    ScopeGuard(const ScopeGuard &) = delete;
    void operator=(const ScopeGuard &) = delete;

private:
    std::function<void()> f;
};

struct RecordedLog {
    logging::Timestamp timestamp;
    logging::Severity severity;
    logging::TaskId task_id;
    const logging::Component *component;
    std::string message;
};

struct ScopedInMemoryLog {
    std::deque<RecordedLog> logs;

    ScopedInMemoryLog();
    ScopedInMemoryLog(ScopedInMemoryLog &&other) = delete;
    ~ScopedInMemoryLog();
};

ScopeGuard with_log_platform_task_id_get(std::optional<std::function<logging::TaskId()>> func);
ScopeGuard with_log_platform_timestamp_get(std::optional<std::function<logging::Timestamp()>> func);
extern logging::Destination in_memory_log;
