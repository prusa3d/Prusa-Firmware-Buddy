#include "sleep.hpp"
#include "printer.hpp"
#include "planner.hpp"

#include <common/timing.h>
#include <common/bsod.h>

#include <cmsis_os.h>

#include <debug.h>
#include <cinttypes>

LOG_COMPONENT_REF(connect);

using http::Connection;
using std::min;
using std::nullopt;
using std::optional;
using transfers::Transfer;

namespace connect_client {

namespace {

    // Wait a bit between config retries, looking at incoming data, etc.
    const constexpr uint32_t IDLE_WAIT = 100;

    enum class TaskResult {
        RerunSoon,
        RerunLater,
        // Do not run this task again, but continue sleeping.
        TaskDone,
        // Wake up / terminate this sleep (after finishing all the tasks at least once).
        WakeUp,
        // Terminate this sleep right now, without running further tasks in the
        // list.
        Reschedule,
    };

    class Task {
    public:
        virtual ~Task() = default;
        virtual TaskResult step() = 0;
    };

    class ManyTasks final : public Task {
    private:
        Task **subtasks;
        size_t task_count;

    public:
        ManyTasks(Task **subtasks, size_t task_count)
            : subtasks(subtasks)
            , task_count(task_count) {}

        virtual TaskResult step() override {
            TaskResult result = TaskResult::TaskDone;

            // Not a for cycle, because we either increment i XOR decrement task_count in each iteration.
            size_t i = 0;
            while (i < task_count) {
                TaskResult sub_result = subtasks[i]->step();

                bool eradicate = false;

                switch (sub_result) {
                case TaskResult::RerunSoon:
                case TaskResult::RerunLater:
                    // Wakeup takes precedence over both reruns.
                    // RerunSoon taske precedence over RerunLater.
                    // For code simplicity, we allow RerunLater to overwrite itself.
                    if (result != TaskResult::WakeUp && result != TaskResult::RerunSoon) {
                        result = sub_result;
                    }
                    break;
                case TaskResult::TaskDone:
                    eradicate = true;
                    break;
                case TaskResult::WakeUp:
                    result = TaskResult::WakeUp;
                    break;
                case TaskResult::Reschedule:
                    return TaskResult::Reschedule;
                }

                if (eradicate) {
                    // Eradicate this task (memmove is fine, it's a pointer)
                    // Preserve the order in case it matters
                    memmove(subtasks + i, subtasks + i + 1, (task_count - i - 1) * sizeof(subtasks[0]));
                    task_count--;
                } else {
                    i++;
                }
            }

            return result;
        }
    };

    class ConfigChanged final : public Task {
    private:
        Printer &printer;

    public:
        ConfigChanged(Printer &printer)
            : printer(printer) {}
        virtual TaskResult step() override {
            // Early check. If during any iteration the config changes, we abort
            // the sleep and let the outer loop deal with that.
            //
            // Do _not_ reset the "changed" flag, we leave that for the outer loop.
            if (get<1>(printer.config(false))) {
                return TaskResult::Reschedule;
            } else {
                return TaskResult::RerunLater;
            }
        }
    };

    class TransferRecovery final : public Task {
    private:
        Planner &planner;

    public:
        TransferRecovery(Planner &planner)
            : planner(planner) {}
        virtual TaskResult step() override {
            // out-variable
            Transfer::Path path;
            switch (Transfer::search_transfer_for_recovery(path)) {
            case Transfer::RecoverySearchResult::NothingToRecover:
                planner.transfer_recovery_finished(nullopt);
                return TaskResult::WakeUp;
                break;
            case Transfer::RecoverySearchResult::Success:
                planner.transfer_recovery_finished(path.as_destination());
                return TaskResult::WakeUp;
                break;
            case Transfer::RecoverySearchResult::WaitingForUSB:
                return TaskResult::RerunLater;
            }

            assert(0); // Unreachable
            return TaskResult::TaskDone;
        }
    };

    class TransferCleanup final : public Task {
    private:
        Planner &planner;

    public:
        TransferCleanup(Planner &planner)
            : planner(planner) {}
        virtual TaskResult step() override {
            planner.transfer_cleanup_finished(Transfer::cleanup_transfers());
            // No matter if we want to re-run the cleanup, we don't do it in
            // this sleep. Possibly in the next one.
            return TaskResult::TaskDone;
        }
    };

    class SubmitBackgroundCmd final : public Task {
    private:
        BackgroundCmd *background_cmd;
        Printer &printer;
        Planner &planner;

    public:
        SubmitBackgroundCmd(BackgroundCmd *background_cmd, Printer &printer, Planner &planner)
            : background_cmd(background_cmd)
            , printer(printer)
            , planner(planner) {}
        virtual TaskResult step() override {
            assert(background_cmd != nullptr);
            switch (auto result = background_cmd_step(*background_cmd, printer); result) {
            case BackgroundResult::More:
                // Try another iteration (maybe, if there's time).
                return TaskResult::RerunSoon;
            case BackgroundResult::Success:
            case BackgroundResult::Failure:
                planner.background_done(result);
                // Something likely changed as result of the background
                // command being finished, stop sleeping and let the
                // outer loop deal with it.
                return TaskResult::WakeUp;
            case BackgroundResult::Later:
                // We have more work to do, but can't do it now. Maybe a bit later?
                return TaskResult::RerunLater;
            }

            assert(0); // Unreachable
            return TaskResult::TaskDone;
        }
    };

    class TransferWatch final : public Task {
    private:
        Transfer *transfer;
        Printer &printer;
        Planner &planner;

    public:
        TransferWatch(Transfer *transfer, Printer &printer, Planner &planner)
            : transfer(transfer)
            , printer(printer)
            , planner(planner) {}
        virtual TaskResult step() override {
            assert(transfer != nullptr);

            switch (auto result = transfer->step(printer.is_printing()); result) {
            case Transfer::State::Downloading:
                // The downloading is happening in another thread, we just
                // watch it from here. No need to do so in a busy-loop.
                return TaskResult::RerunLater;
            case Transfer::State::Retrying:
                // Go for another iteration (now or during next sleep).
                return TaskResult::RerunSoon;
            case Transfer::State::Failed:
            case Transfer::State::Finished:
                planner.download_done(result);
                // Something likely changed as a result of the download
                // being done, let the outer loop deal with that.
                return TaskResult::WakeUp;
            }

            assert(0); // Unreachable
            return TaskResult::TaskDone;
        }
    };

    class ReadableConnection final : public Task {
    private:
        Connection *connection;
        Planner &planner;

    public:
        ReadableConnection(Connection *connection, Planner &planner)
            : connection(connection)
            , planner(planner) {}
        virtual TaskResult step() override {
            if (connection->poll_readable(0)) {
                planner.notify_command_waiting();
                return TaskResult::WakeUp;
            } else {
                return TaskResult::RerunLater;
            }
        }
    };

} // namespace

Timestamp now() {
    return ticks_ms();
}

void sleep_raw(Duration sleep_for) {
    osDelay(sleep_for);
}

Sleep Sleep::idle(Duration sleep_for) {
    return Sleep(sleep_for, nullptr, nullptr, nullptr, false, false);
}

void Sleep::perform(Printer &printer, Planner &planner) {
    log_debug(connect, "Sleeping up to %" PRIu32 " milliseconds", milliseconds);
    // * Config
    // * Transfer recovery or cleanup (not both)
    // * Background command
    // * Transfer
    // * A connection to watch
    constexpr size_t MAX_TASKS = 5;
    Task *to_perform[MAX_TASKS];
    size_t to_perform_cnt = 0;

    auto add = [&](Task *task) {
        if (to_perform_cnt >= MAX_TASKS) {
            bsod("Adjust max number of tasks");
        }

        to_perform[to_perform_cnt++] = task;
    };

    ConfigChanged config_changed(printer);
    add(&config_changed);

    TransferRecovery recovery(planner);
    TransferCleanup cleanup(planner);
    if (run_transfer_recovery) {
        add(&recovery);
    } else if (cleanup_transfers) {
        // Note: We don't want to do both the recovery and cleanup in the same
        // sleep ‒ we don't want to do the cleanup before recovery.
        add(&cleanup);
    }

    SubmitBackgroundCmd command(background_cmd, printer, planner);
    if (background_cmd != nullptr) {
        add(&command);
    }

    TransferWatch transfer_watch(transfer, printer, planner);
    if (transfer != nullptr) {
        add(&transfer_watch);
    }

    ReadableConnection connection(wake_on_readable, planner);
    if (wake_on_readable != nullptr) {
        add(&connection);
    }

    // We want to give each task a chance to run at least once even if we sleep for 0ms.
    bool need_run = to_perform_cnt > 0;

    ManyTasks tasks(to_perform, to_perform_cnt);

    Timestamp before = now();

    while (milliseconds > 0 || need_run) {
        need_run = false;

        TaskResult result = tasks.step();
        Timestamp after = now();

        // May underflow, but will yield the right result.
        Duration took = after - before;

        // min to deal with the action taking longer that expected/allowed.
        milliseconds -= min(milliseconds, took);

        bool do_sleep = false;

        switch (result) {
        case TaskResult::RerunSoon:
            // Just let the next iteration run
            break;
        case TaskResult::RerunLater: // Sleep between two iterations
        case TaskResult::TaskDone: // Nothing to run, just sleep
            do_sleep = true;
            break;
        case TaskResult::WakeUp:
        case TaskResult::Reschedule:
            log_debug(connect, "Early wakeup");
            return;
        }

        if (took >= IDLE_WAIT) {
            // The iteration took longer than we are allowed, abort sleeping.
            do_sleep = false;
        }

        if (do_sleep) {
            Duration left = min(milliseconds, IDLE_WAIT - took);
            sleep_raw(left);

            Timestamp after_sleep = now();
            took = after_sleep - after;
            milliseconds -= min(milliseconds, took);

            before = after_sleep;
        } else {
            before = after;
        }
    }

    log_debug(connect, "Sleep done");
}

} // namespace connect_client
