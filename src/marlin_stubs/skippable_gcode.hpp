#pragma once

#include <atomic>

/// A general API for gcode operations that can be skipped from the GUI/connect/...
class SkippableGCode {

public:
    class Guard {

    public:
        Guard();
        Guard(Guard &&) = delete;
        Guard(const Guard &) = delete;
        ~Guard();

        /// \returns whether the currently running operation was marked for skipping
        bool is_skip_requested() const;
    };

    /// \returns whether we're currently running a gcode operation that can be skipped
    /// Can be called from any thread
    bool is_running() const;

    /// Requests skip of the currently running operation.
    /// Can be called from any thread.
    void request_skip();

private:
    std::atomic<bool> is_running_ = false;
    std::atomic<bool> skip_requested_ = false;
};

SkippableGCode &skippable_gcode();
