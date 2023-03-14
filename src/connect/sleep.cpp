#include "sleep.hpp"
#include "printer.hpp"
#include "planner.hpp"

#include <common/timing.h>

#include <cmsis_os.h>

using std::min;
using std::nullopt;
using std::optional;
using transfers::DownloadStep;

namespace connect_client {

namespace {

    // Wait half a second between config retries and similar.
    const constexpr uint32_t IDLE_WAIT = 500;

    const constexpr uint32_t COMMAND_LATER_TIME = 100;

    enum class Mode {
        Background,
        Download,
        Delay,
    };

}

Timestamp now() {
    return ticks_ms();
}

void sleep_raw(Duration sleep_for) {
    osDelay(sleep_for);
}

Sleep Sleep::idle() {
    return Sleep(IDLE_WAIT, nullptr, nullptr, false);
}

void Sleep::perform(Printer &printer, Planner &planner) {
    // Make sure we perform the background task at least once during each
    // sleep, even if the sleep would be 0.
    bool need_background = background_cmd != nullptr;
    bool need_download = download != nullptr;
    // Which one takes a turn in case both are active.
    bool prefer_download = true;

    while (need_background || need_download || milliseconds > 0) {
        Timestamp before = now();

        // Early check. If during any iteration the config changes, we abort
        // the sleep and let the outer loop deal with that.
        //
        // Do _not_ reset the "changed" flag, we leave that for the outer loop.
        if (get<1>(printer.config(false))) {
            return;
        }

        // We want to interrupt the sleep periodically and check that config
        // doesn't change.
        Duration max_step_time = min(milliseconds, IDLE_WAIT);

        Mode mode = Mode::Delay;
        if (background_cmd != nullptr && download != nullptr) {
            // In case we have both, we want each to do as much work without
            // blocking the other one. So we give it 0 timeout.
            //
            // They are going to take turns.
            max_step_time = 0;
            if (prefer_download) {
                mode = Mode::Download;
                prefer_download = false;
            } else {
                mode = Mode::Background;
                prefer_download = true;
            }
        } else if (background_cmd != nullptr) {
            mode = Mode::Background;
        } else if (download != nullptr) {
            mode = Mode::Download;
        }

        optional<Duration> time_cap = nullopt;

        switch (mode) {
        case Mode::Background:
            // Performed at least one background step.
            need_background = false;
            assert(background_cmd != nullptr);
            switch (auto result = step(*background_cmd, printer); result) {
            case BackgroundResult::More:
                // Try another iteration (maybe, if there's time).
                break;
            case BackgroundResult::Success:
            case BackgroundResult::Failure:
                planner.background_done(result);
                background_cmd = nullptr;
                // Something likely changed as result of the background
                // command being finished, stop sleeping and let the
                // outer loop deal with it.
                return;
            case BackgroundResult::Later:
                // The background command wants to do something, but
                // not now. So we leave it be for this sleep, but make
                // sure we don't sleep _too long_.
                time_cap = COMMAND_LATER_TIME;
                background_cmd = nullptr;
            }
            break;
        case Mode::Download:
            // Performed at least one download step.
            need_download = false;
            assert(download != nullptr);

            if (recover_download) {
                planner.recover_download();
                // An attempt to recover can actually "kill" the download
                // completely. Therefore, just bail out and let the outer loop
                // figure everything out.
                return;
            }

            switch (auto result = download->step(max_step_time); result) {
            case DownloadStep::Continue:
                // Go for another iteration (now or during next sleep).
                break;
            case DownloadStep::FailedNetwork:
            case DownloadStep::FailedOther:
            case DownloadStep::Finished:
                planner.download_done(result);
                download = nullptr;
                // Something likely changed as a result of the download
                // being done, let the outer loop deal with that.
                return;
            }
            break;
        case Mode::Delay:
            sleep_raw(max_step_time);
            break;
        }

        Timestamp after = now();

        // May underflow, but will yield the right result.
        Duration took = after - before;

        // min to deal with the action taking longer that expected/allowed.
        milliseconds -= min(milliseconds, took);

        if (time_cap.has_value()) {
            milliseconds = min(milliseconds, *time_cap);
        }
    }
}

}
