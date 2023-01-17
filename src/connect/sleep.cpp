#include "sleep.hpp"
#include "printer.hpp"
#include "planner.hpp"

#include <common/timing.h>

#include <cmsis_os.h>

using std::min;
using std::nullopt;
using std::optional;

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

Sleep Sleep::idle() {
    return Sleep(IDLE_WAIT, nullptr);
}

void Sleep::perform(Printer &printer, Planner &planner) {
    // Make sure we perform the background task at least once during each
    // sleep, even if the sleep would be 0.
    bool need_background = background_cmd != nullptr;
    bool need_download = false;
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
        if (background_cmd != nullptr) {
            mode = Mode::Background;
        }

        // TODO Download processing.
        // TODO These should be taking turns.
        // TODO If both Background and Download are present, set the step duration to 0.

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
            // XXX
            break;
        case Mode::Delay:
            osDelay(max_step_time);
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
