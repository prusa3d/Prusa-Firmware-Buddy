#pragma once
#include <optional>
#include <functional>
#include <vector>
#include <deque>
#include <string>
#include "log.h"

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
    log_timestamp_t timestamp;
    log_severity_t severity;
    log_task_id_t task_id;
    const log_component_t *component;
    std::string message;
};

struct ScopedInMemoryLog {
    std::deque<RecordedLog> logs;

    ScopedInMemoryLog();
    ScopedInMemoryLog(ScopedInMemoryLog &&other) = delete;
    ~ScopedInMemoryLog();
};

ScopeGuard with_log_platform_task_id_get(std::optional<std::function<log_task_id_t()>> func);
ScopeGuard with_log_platform_timestamp_get(std::optional<std::function<log_timestamp_t()>> func);
extern log_destination_t in_memory_log;
