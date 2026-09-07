///@file
#include <rtt_metrics_task/rtt_metrics_task.hpp>

#include <freertos/timing.hpp>
#include <rtt_metrics_segger/peripheries_metrics.hpp>
#include <rtt_metrics_segger/rtt_metrics_segger.hpp>
#include <rtt_metrics_structs/rtt_metrics_structs.hpp>

#include <Marlin/src/module/stepper.h>

static_assert(rtt_metrics::stepper_count == XYZE_N,
    "StepperPositions wire layout is out of sync with the firmware stepper count (XYZE_N)");

void rtt_metrics::capture_stepper_positions() {
    // Samples are in steps
    rtt_metrics::StepperPositions positions;
    positions.steps[0] = Stepper::position_from_startup(X_AXIS);
    positions.steps[1] = Stepper::position_from_startup(Y_AXIS);
    positions.steps[2] = Stepper::position_from_startup(Z_AXIS);
    positions.steps[3] = Stepper::position_from_startup(E_AXIS);
    sample_stepper_positions(positions);
}

void rtt_metrics_task() {
    // Poll cadence for draining the metric queues. Kept short so the queues
    // (see peripheries_metrics.cpp) stay well within their depth at the
    // producers' sampling rates.
    constexpr size_t poll_interval_ms = 1;

    rtt_metrics::init_rtt_metrics();
    for (;;) {
        rtt_metrics::process_rtt_metrics_queue();
        freertos::delay(poll_interval_ms);
    }
}
