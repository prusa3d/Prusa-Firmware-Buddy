#include <catch2/catch_test_macros.hpp>

#include <signal_processing/pipeline.hpp>
#include <module/signal2step.hpp>
#include <algorithm>
#include <bit>
#include <limits>
#include <utility>
#include <vector>
#include <cmath>

namespace manual = precise_stepping::manual;
using signal2step::Event;

// Helper structure to collect step events
struct StepRecord {
    uint32_t timestamp_us;
    AxisEnum axis;
    bool direction;
};

// Every test starts from an empty queue, a flat bed and 1mm steps.
static void reset_stubs() {
    manual::stub_step_events.clear();
    manual::stub_queue_capacity = std::numeric_limits<size_t>::max();
    manual::stub_released_after.reset();
    PreciseStepping::stop_pending = false;
    stub_mesh_z_correction = nullptr;
    for (auto &mm : Planner::mm_per_step) {
        mm = 1;
    }
}

// The steps as the step ISR would execute them: each event is a delay after
// the previous one.
static std::vector<StepRecord> recorded_steps() {
    std::vector<StepRecord> steps;
    uint32_t timestamp_us = 0;
    for (const auto &event : manual::stub_step_events) {
        timestamp_us += event.delay_us;
        if (const unsigned axis_bits = event.flags & STEP_EVENT_FLAG_AXIS_MASK) {
            steps.push_back({ timestamp_us, AxisEnum(std::countr_zero(axis_bits)), !event.dir });
        }
    }
    return steps;
}

static uint32_t recorded_duration_us() {
    uint32_t duration_us = 0;
    for (const auto &event : manual::stub_step_events) {
        duration_us += event.delay_us;
    }
    return duration_us;
}

// Runs the trajectory with the machine resting at its first sample.
static std::vector<StepRecord> run(const std::vector<xyze_pos_t> &data, float sampling_freq = 1000.0f) {
    Planner::stub_position = to_machine_pos(data.front());
    signal2step::run_trajectory(sp::pipe::make_source(data, sampling_freq) | sp::pipe::type_erase_auto(), [](Event, uint32_t) {});
    return recorded_steps();
}

TEST_CASE("Signal2Step - Single axis forward movement", "[signal2step]") {
    reset_stubs();

    // X moves from 0mm to 4mm in 3 samples at 1000 Hz
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 2, 0, 0, 0 },
        { 4, 0, 0, 0 },
    });

    // Should generate 4 steps on X axis (from step 0 to step 4)
    REQUIRE(steps.size() == 4);

    for (const auto &step : steps) {
        REQUIRE(step.axis == X_AXIS);
        REQUIRE(step.direction == true); // Forward
    }

    // Steps should be sorted by timestamp
    for (size_t i = 1; i < steps.size(); ++i) {
        REQUIRE(steps[i].timestamp_us >= steps[i - 1].timestamp_us);
    }
}

TEST_CASE("Signal2Step - Single axis backward movement", "[signal2step]") {
    reset_stubs();

    // Y moves from 5mm to 2mm (3 steps backward)
    const auto steps = run({
        { 0, 5, 0, 0 },
        { 0, 2, 0, 0 },
    });

    REQUIRE(steps.size() == 3);

    for (const auto &step : steps) {
        REQUIRE(step.axis == Y_AXIS);
        REQUIRE(step.direction == false); // Backward
    }
}

TEST_CASE("Signal2Step - Multi-axis movement with sorted timestamps", "[signal2step]") {
    reset_stubs();

    // Both X and Y move simultaneously
    // X: 0 -> 3mm (3 steps)
    // Y: 0 -> 2mm (2 steps)
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 3, 2, 0, 0 },
    });

    // Should generate 5 steps total (3 on X, 2 on Y)
    REQUIRE(steps.size() == 5);

    // Count steps per axis
    int x_steps = 0, y_steps = 0;
    for (const auto &step : steps) {
        if (step.axis == X_AXIS) {
            x_steps++;
        }
        if (step.axis == Y_AXIS) {
            y_steps++;
        }
    }

    REQUIRE(x_steps == 3);
    REQUIRE(y_steps == 2);

    // All steps should be in sorted order by timestamp
    for (size_t i = 1; i < steps.size(); ++i) {
        REQUIRE(steps[i].timestamp_us >= steps[i - 1].timestamp_us);
    }

    // All timestamps should be within the interval [0, 1000]us
    for (const auto &step : steps) {
        REQUIRE(step.timestamp_us <= 1000);
    }
}

TEST_CASE("Signal2Step - Interpolated timestamps", "[signal2step]") {
    reset_stubs();

    // X moves from 0 to 4mm in one interval
    // With 1mm per step, we expect 4 steps
    // At 1000Hz, interval is 1000us
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 4, 0, 0, 0 },
    });

    REQUIRE(steps.size() == 4);

    // Steps should be roughly evenly spaced
    // Expected at approximately: 125us, 375us, 625us, 875us
    // (when crossing 0.5, 1.5, 2.5, 3.5)
    REQUIRE(steps[0].timestamp_us < 200); // ~125us
    REQUIRE(steps[1].timestamp_us > 300);
    REQUIRE(steps[1].timestamp_us < 450); // ~375us
    REQUIRE(steps[2].timestamp_us > 550);
    REQUIRE(steps[2].timestamp_us < 700); // ~625us
    REQUIRE(steps[3].timestamp_us > 800); // ~875us
}

TEST_CASE("Signal2Step - No movement generates no steps", "[signal2step]") {
    reset_stubs();

    // All axes stay at 0
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    });

    REQUIRE(steps.empty());
}

TEST_CASE("Signal2Step - Fractional movement less than one step", "[signal2step]") {
    reset_stubs();

    // X moves by 0.4mm (less than 1 step)
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 0.4f, 0, 0, 0 },
    });

    // No steps should be generated (0.4mm rounds to 0 steps)
    REQUIRE(steps.empty());
}

TEST_CASE("Signal2Step - Different mm_per_step values", "[signal2step]") {
    reset_stubs();

    // X moves 10mm with 2mm per step = 5 steps
    // Y moves 10mm with 1mm per step = 10 steps
    Planner::mm_per_step[X_AXIS] = 2;
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 10, 10, 0, 0 },
    });

    // Count steps per axis
    int x_steps = 0, y_steps = 0;
    for (const auto &step : steps) {
        if (step.axis == X_AXIS) {
            x_steps++;
        }
        if (step.axis == Y_AXIS) {
            y_steps++;
        }
    }

    REQUIRE(x_steps == 5); // 10mm / 2mm per step
    REQUIRE(y_steps == 10); // 10mm / 1mm per step
}

TEST_CASE("Signal2Step - Extruder axis", "[signal2step]") {
    reset_stubs();

    // E (extruder) moves from 0 to 3mm
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 0, 0, 0, 3 },
    });

    REQUIRE(steps.size() == 3);

    for (const auto &step : steps) {
        REQUIRE(step.axis == E_AXIS);
        REQUIRE(step.direction == true);
    }
}

TEST_CASE("Signal2Step - All axes moving", "[signal2step]") {
    reset_stubs();

    // All axes move by 2mm
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 2, 2, 2, 2 },
    });

    // Should generate 8 steps total (2 per axis * 4 axes)
    REQUIRE(steps.size() == 8);

    // Count steps per axis
    int x_steps = 0, y_steps = 0, z_steps = 0, e_steps = 0;
    for (const auto &step : steps) {
        if (step.axis == X_AXIS) {
            x_steps++;
        }
        if (step.axis == Y_AXIS) {
            y_steps++;
        }
        if (step.axis == Z_AXIS) {
            z_steps++;
        }
        if (step.axis == E_AXIS) {
            e_steps++;
        }
        REQUIRE(step.direction == true); // All forward
    }

    REQUIRE(x_steps == 2);
    REQUIRE(y_steps == 2);
    REQUIRE(z_steps == 2);
    REQUIRE(e_steps == 2);

    // Steps should be sorted by timestamp
    for (size_t i = 1; i < steps.size(); ++i) {
        REQUIRE(steps[i].timestamp_us >= steps[i - 1].timestamp_us);
    }
}

TEST_CASE("Signal2Step - Multiple samples with continuous movement", "[signal2step]") {
    reset_stubs();

    // X moves 1mm per sample for 5 samples
    const auto steps = run({
        { 0, 0, 0, 0 },
        { 1, 0, 0, 0 },
        { 2, 0, 0, 0 },
        { 3, 0, 0, 0 },
        { 4, 0, 0, 0 },
    });

    // Should generate 4 steps (from 0 to 4)
    REQUIRE(steps.size() == 4);

    // Timestamps should span multiple sample intervals
    REQUIRE(steps[0].timestamp_us < 1000); // First interval
    REQUIRE(steps[1].timestamp_us >= 1000); // Second interval
    REQUIRE(steps[1].timestamp_us < 2000);
    REQUIRE(steps[2].timestamp_us >= 2000); // Third interval
    REQUIRE(steps[2].timestamp_us < 3000);
    REQUIRE(steps[3].timestamp_us >= 3000); // Fourth interval
}

// The motion starts at the first event, whatever its delay, and is over at the
// last one, so the queue has to start with a zero-delay event and run to the
// end of the signal even through samples without steps.
TEST_CASE("Signal2Step - The queue spans the whole signal", "[signal2step]") {
    reset_stubs();

    const std::vector<xyze_pos_t> data = {
        { 0, 0, 0, 0 },
        { 1, 0, 0, 0 },
        { 1, 0, 0, 0 },
        { 1, 0, 0, 0 },
    };
    Planner::stub_position = data.front();

    int starts = 0;
    uint32_t start_us = 0;
    signal2step::run_trajectory(sp::pipe::make_source(data, 1000.0f) | sp::pipe::type_erase_auto(), [&](Event event, uint32_t motion_start_us) {
        if (event == Event::motion_start) {
            start_us = motion_start_us;
            starts++;
        }
    });

    // A signal that fits the queue is queued whole before the motion starts.
    REQUIRE(starts == 1);
    REQUIRE(manual::stub_released_after == manual::stub_step_events.size());
    REQUIRE(start_us == 100);

    const auto &events = manual::stub_step_events;
    REQUIRE(events.front().delay_us == 0);
    REQUIRE(events.front().flags == STEP_EVENT_FLAG_KEEP_ALIVE);
    REQUIRE((events.back().flags & STEP_EVENT_FLAG_END_OF_MOTION));

    REQUIRE(recorded_steps().size() == 1);
    REQUIRE(recorded_duration_us() == 3000);
}

// A step event carries at most a 16-bit delay, so longer gaps are bridged.
TEST_CASE("Signal2Step - Long gaps are split into keep-alives", "[signal2step]") {
    reset_stubs();

    // At 5 Hz the single step falls 100ms into the only interval.
    const auto steps = run({
                               { 0, 0, 0, 0 },
                               { 1, 0, 0, 0 },
                           },
        5.0f);

    for (const auto &event : manual::stub_step_events) {
        REQUIRE(event.delay_us <= STEP_TIMER_MAX_TICKS_LIMIT);
    }
    REQUIRE(steps.size() == 1);
    REQUIRE(steps[0].timestamp_us == 100'000);
    REQUIRE(recorded_duration_us() == 200'000);
}

// The motion starts only once the queue is full, so that no event can be
// queued after it is due.
TEST_CASE("Signal2Step - Motion starts with a full queue", "[signal2step]") {
    reset_stubs();

    const std::vector<xyze_pos_t> data = {
        { 0, 0, 0, 0 },
        { 10, 0, 0, 0 },
    };
    Planner::stub_position = data.front();
    manual::stub_queue_capacity = 3;

    std::vector<Event> events;
    signal2step::run_trajectory(sp::pipe::make_source(data, 1000.0f) | sp::pipe::type_erase_auto(), [&](Event event, uint32_t) {
        events.push_back(event);
        if (event == Event::step_queue_full) {
            // The ISR drains the queue.
            manual::stub_queue_capacity = std::numeric_limits<size_t>::max();
        }
    });

    REQUIRE(manual::stub_released_after == 3);
    REQUIRE(events.size() >= 2);
    REQUIRE(events[0] == Event::motion_start);
    REQUIRE(events[1] == Event::step_queue_full);
    REQUIRE(std::ranges::count(events, Event::motion_start) == 1);
    REQUIRE(recorded_steps().size() == 10);
}

// A source that has no sample ready for its first `stalls` polls, as a host
// stream that has not arrived yet.
struct StallingSource {
    using sample_type = xyze_pos_t;

    std::vector<xyze_pos_t> data;
    int &stalls;
    size_t at = 0;

    sp::pipe::PollResult poll() {
        if (stalls > 0) {
            return sp::pipe::PollResult::pending;
        }
        return at < data.size() ? sp::pipe::PollResult::ready : sp::pipe::PollResult::done;
    }

    xyze_pos_t next() { return data[at++]; }

    sp::SamplingFreq sampling_freq() const { return 1000.0f; }
};

// Waiting for samples at the start delays the start, not the motion.
TEST_CASE("Signal2Step - Missing samples hold the start", "[signal2step]") {
    reset_stubs();

    const std::vector<xyze_pos_t> data = {
        { 0, 0, 0, 0 },
        { 10, 0, 0, 0 },
    };
    Planner::stub_position = data.front();
    manual::stub_queue_capacity = 3;

    int stalls = 5;
    std::vector<std::pair<Event, uint32_t>> events;
    signal2step::run_trajectory(StallingSource { data, stalls } | sp::pipe::type_erase_auto(), [&](Event event, uint32_t motion_start_us) {
        events.emplace_back(event, motion_start_us);
        if (event == Event::no_signal_samples) {
            stalls--;
        }
        if (event == Event::step_queue_full) {
            manual::stub_queue_capacity = std::numeric_limits<size_t>::max();
        }
    });

    REQUIRE(manual::stub_released_after == 3);
    REQUIRE(events.size() > 5);
    for (int i = 0; i < 5; i++) {
        REQUIRE(events[i] == std::pair { Event::no_signal_samples, uint32_t { 0 } });
    }
    REQUIRE(events[5] == std::pair { Event::motion_start, uint32_t { 100 } });
    REQUIRE(recorded_steps().size() == 10);
}

// A stop before the start still starts the motion once, with nothing queued.
TEST_CASE("Signal2Step - A quick stop before the start", "[signal2step]") {
    reset_stubs();

    const std::vector<xyze_pos_t> data = {
        { 0, 0, 0, 0 },
        { 10, 0, 0, 0 },
    };
    Planner::stub_position = data.front();

    int stalls = 1;
    int starts = 0;
    signal2step::run_trajectory(StallingSource { data, stalls } | sp::pipe::type_erase_auto(), [&](Event event, uint32_t) {
        if (event == Event::no_signal_samples) {
            PreciseStepping::stop_pending = true;
            stalls--;
        }
        if (event == Event::motion_start) {
            starts++;
        }
        if (event == Event::flushing_steps) {
            PreciseStepping::stop_pending = false;
        }
    });

    REQUIRE(starts == 1);
    REQUIRE(manual::stub_released_after.has_value());
    REQUIRE(manual::stub_step_events.size() == 1);
}

// A quick stop resets the step queue; nothing may be enqueued after it.
TEST_CASE("Signal2Step - A quick stop ends the motion", "[signal2step]") {
    reset_stubs();

    const std::vector<xyze_pos_t> data = {
        { 0, 0, 0, 0 },
        { 10, 0, 0, 0 },
        { 20, 0, 0, 0 },
    };
    Planner::stub_position = data.front();
    manual::stub_queue_capacity = 3;

    signal2step::run_trajectory(sp::pipe::make_source(data, 1000.0f) | sp::pipe::type_erase_auto(), [](Event event, uint32_t) {
        if (event == Event::step_queue_full) {
            PreciseStepping::stop_pending = true;
        }
        if (event == Event::flushing_steps) {
            // Handling the stop resets the queue.
            PreciseStepping::stop_pending = false;
        }
    });

    // The starting event and the two steps that fitted before the stop.
    REQUIRE(manual::stub_step_events.size() == 3);
    REQUIRE(recorded_steps().size() == 2);
}

// Speeds are integrated from the current position, except E, which is
// relative to the start.
TEST_CASE("Signal2Step - Speed profile starts at the current position", "[signal2step]") {
    reset_stubs();

    current_position = { 10, 20, 5, 1000 };
    Planner::stub_position = current_position;

    // 1mm per sample at 1 kHz, for 4 samples.
    const std::vector<xyze_float_t> speeds(4, xyze_float_t { 1000, 0, 0, 1000 });
    signal2step::run_speed_profile(sp::pipe::make_source(speeds, 1000.0f) | sp::pipe::type_erase_auto(), [](Event, uint32_t) {});

    int x_steps = 0, e_steps = 0;
    for (const auto &step : recorded_steps()) {
        REQUIRE(step.direction);
        x_steps += step.axis == X_AXIS;
        e_steps += step.axis == E_AXIS;
    }
    REQUIRE(x_steps == 4);
    REQUIRE(e_steps == 4);
}

// A mesh rising along X has to be stepped out on top of whatever Z the
// trajectory asked for, measured at the absolute position each sample lands on.
TEST_CASE("Signal2Step - Bed levelling shapes Z", "[signal2step]") {
    reset_stubs();

    // 0.5mm of mesh per mm of X, zero at the start's X of 10.
    stub_mesh_z_correction = [](const xyze_pos_t &pos) { return 0.5f * (pos.x - 10.f); };

    // A pure X move. No commanded Z at all.
    const auto steps = run({
        { 10, 20, 5, 0 },
        { 12, 20, 5, 0 },
        { 14, 20, 5, 0 },
    });

    int x_steps = 0, z_steps = 0;
    for (const auto &step : steps) {
        REQUIRE(step.direction);
        x_steps += step.axis == X_AXIS;
        z_steps += step.axis == Z_AXIS;
    }

    // 4mm of X, and the 2mm of Z the mesh adds across it.
    REQUIRE(x_steps == 4);
    REQUIRE(z_steps == 2);
}
