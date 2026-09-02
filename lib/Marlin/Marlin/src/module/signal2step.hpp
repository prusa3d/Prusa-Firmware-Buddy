#pragma once

// This module executes a trajectory sampled at fixed frequency by generating
// the stepper motor steps directly, bypassing the planner.

#include <signal_processing/pipeline.hpp>
#include <core/types.h>
#include <core/mtypes.hpp>

#include <Marlin.h>
#include <feature/phase_stepping/phase_stepping.hpp>
#include <feature/precise_stepping/manual.hpp>
#include <module/motion.h>
#include <module/planner.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <bsod/bsod.h>

namespace signal2step {

// One position per motor axis is what a sample carries throughout this module.
constexpr int STEPPER_AXES = 4;

/// Why run_trajectory() / run_speed_profile() handed control to the caller.
/// run_*() never idles on its own: every handler call must run the Marlin
/// loop (idle(true) or an equivalent), and must return well within the time
/// already in the step queue, or the motion stutters. Never enqueue steps or
/// touch the planner from the handler. Every call also gets the moment the
/// motion starts, in ticks_us(), or 0 before motion_start.
enum class Event {
    /// Motion is about to start, at the moment passed along; the handler is
    /// called once per run. The step queue is filled ahead first, so only
    /// no_signal_samples can come before it, however long that takes. Start
    /// whatever has to be aligned with the motion (sensor recording).
    motion_start,

    /// The signal has no sample ready. Feed the source (e.g. read the host
    /// stream), then idle. Before motion_start, this delays the start rather
    /// than the motion.
    no_signal_samples,

    /// The step queue is full. Service time-critical work (drain sensor
    /// queues, report progress), then idle.
    step_queue_full,

    /// The whole trajectory is queued and is being executed. Keep servicing
    /// sensors, then idle; the handler keeps being called until the motion
    /// has finished.
    flushing_steps,
};

template <typename F>
concept EventHandler = requires(F f, Event e, uint32_t motion_start_us) {
    { f(e, motion_start_us) } -> std::same_as<void>;
};

namespace detail {

// Maps array index to AxisEnum for step callbacks.
inline AxisEnum idx_to_axis(std::size_t idx) {
    static_assert(X_AXIS == 0 && Y_AXIS == 1 && Z_AXIS == 2 && E_AXIS == 3,
        "AxisEnum values must match array indices");
    debug_assert(idx <= E_AXIS);
    return static_cast<AxisEnum>(idx);
}

// Tracks per-axis step generation state within a single sample interval. The
// problem is that we need to compute the time between steps based on approximate
// position deltas, which may not align with step positions. This struct tracks
// the number of steps left to generate for an axis within the current sample
// interval, the direction, and the next step time.
struct AxisSteps {
    int steps_left = 0;
    bool direction = true;
    uint32_t next_step_timestamp_us = 0;
    uint32_t step_period_us = 0;

    void init(
        float start_pos,
        float end_pos,
        float mm_step,
        float inv_mm_step,
        uint32_t current_time_us,
        float dt_us) {

        debug_assert(dt_us > 0.f);
        debug_assert(mm_step > 0.f);
        debug_assert(inv_mm_step > 0.f);

        const int start_step = static_cast<int>(std::round(start_pos * inv_mm_step));
        const int end_step = static_cast<int>(std::round(end_pos * inv_mm_step));

        if (start_step == end_step) {
            steps_left = 0;
            return;
        }

        const float delta = end_pos - start_pos;
        const float dir = (delta > 0.0f) ? 1.0f : -1.0f;
        direction = (delta > 0.0f);

        steps_left = std::abs(end_step - start_step);
        const float abs_delta = dir * delta;

        // Offset to the first step: when linear motion crosses the first
        // half-step boundary (between step bins), clamped to [0, dt_us].
        const float step_pos = (static_cast<float>(start_step) + dir * 0.5f) * mm_step;
        const float offset = std::clamp(dir * (step_pos - start_pos) / abs_delta * dt_us, 0.0f, dt_us);
        next_step_timestamp_us = current_time_us + static_cast<uint32_t>(offset);

        // Time between subsequent steps under linear motion, clamped to dt_us.
        step_period_us = static_cast<uint32_t>(std::min(dt_us * mm_step / abs_delta, dt_us));
    }

    bool has_steps() const {
        return steps_left > 0;
    }

    void advance() {
        steps_left--;
        next_step_timestamp_us += step_period_us;
    }
};

/// Receives the output of generate_steps(). Timestamps are absolute
/// microseconds since the first sample.
template <typename S>
concept StepSink = requires(S s, uint32_t timestamp_us, AxisEnum axis, bool direction) {
    { s.step(timestamp_us, axis, direction) } -> std::same_as<void>;
    { s.sample_end(timestamp_us) } -> std::same_as<void>;
    { s.wait_for_samples() } -> std::same_as<void>;
    { s.aborted() } -> std::same_as<bool>;
};

/// Pulls motor-coordinate samples and turns the difference between consecutive
/// ones into individually timed steps. The first sample is the starting position
/// and generates no steps. Stops early once the sink reports an abort.
template <sp::pipe::Source Signal, StepSink Sink>
void generate_steps(Signal signal, const std::array<float, STEPPER_AXES> &mm_per_step, Sink &sink) {
    debug_assert(signal.sampling_freq() > 0.f);

    std::array<float, STEPPER_AXES> inv_mm_per_step;
    for (int i = 0; i < STEPPER_AXES; i++) {
        debug_assert(mm_per_step[i] > 0.f);
        inv_mm_per_step[i] = 1.0f / mm_per_step[i];
    }

    abce_pos_t prev_sample = {};

    const uint32_t time_per_sample_us = static_cast<uint32_t>(1'000'000.0f / signal.sampling_freq());

    bool first_sample = true;
    uint32_t current_time_us = 0;
    while (!sink.aborted()) {
        auto poll_result = signal.poll();
        if (poll_result == sp::pipe::PollResult::done) {
            break;
        }
        if (poll_result == sp::pipe::PollResult::pending) {
            sink.wait_for_samples();
            continue;
        }

        const abce_pos_t sample = signal.next();

        if (first_sample) {
            prev_sample = sample;
            first_sample = false;
            continue;
        }

        const uint32_t next_sample_timestamp_us = current_time_us + time_per_sample_us;
        const float dt_us = static_cast<float>(next_sample_timestamp_us - current_time_us);
        std::array<detail::AxisSteps, STEPPER_AXES> axis_steps = {};

        for (int i = 0; i < STEPPER_AXES; i++) {
            axis_steps[i].init(
                prev_sample[i],
                sample[i],
                mm_per_step[i],
                inv_mm_per_step[i],
                current_time_us,
                dt_us);
        }

        while (true) {
            int axis_to_step = -1;
            uint32_t earliest_step_time_us = std::numeric_limits<uint32_t>::max();

            for (int i = 0; i < STEPPER_AXES; i++) {
                const auto &axis = axis_steps[i];
                if (!axis.has_steps()) {
                    continue;
                }
                if (axis.next_step_timestamp_us < earliest_step_time_us) {
                    earliest_step_time_us = axis.next_step_timestamp_us;
                    axis_to_step = i;
                }
            }

            if (axis_to_step < 0) {
                break;
            }

            auto &axis = axis_steps[axis_to_step];
            axis.advance();
            sink.step(earliest_step_time_us, detail::idx_to_axis(axis_to_step), axis.direction);
        }

        sink.sample_end(next_sample_timestamp_us);

        prev_sample = sample;
        debug_assert(current_time_us <= std::numeric_limits<uint32_t>::max() - time_per_sample_us);
        current_time_us = next_sample_timestamp_us;
    }
}

/// Machine XYZE to motor ABCE (inverse kinematics). It has to undo the
/// planner's forward kinematics exactly, since that is what reads the position
/// back from the steppers once the motion is over.
inline abce_pos_t machine_to_motor(const MachinePosXYZE &pos) {
    abce_pos_t r;
#ifdef COREXY
    r.a = pos.x + pos.y;
    r.b = CORESIGN(pos.x - pos.y);
#else
    r.a = pos.x;
    r.b = pos.y;
#endif
    r.c = pos.z;
    r.e = pos.e;
    return r;
}

// A quick stop resets the step queue under us; any step enqueued after it would
// move the machine after it was told to stop.
inline bool motion_aborted() {
    return planner.draining() || PreciseStepping::stopping();
}

// Feeds generated steps into the manual step queue, which takes each event as a
// delay after the previous one.
template <EventHandler F>
class StepQueue {
public:
    explicit StepQueue(F &on_event)
        : on_event(on_event) {}

    // The motion is held until the queue is full or the signal is over.
    void start() {
        held.emplace();
        push(0, false, StepEventFlag::STEP_EVENT_FLAG_KEEP_ALIVE);
    }

    void notify(Event event) {
        on_event(event, start_us);
    }

    void step(uint32_t timestamp_us, AxisEnum axis, bool direction) {
        push_at(timestamp_us, !direction,
            StepEventFlag::STEP_EVENT_FLAG_STEP_X << int(axis) | StepEventFlag::STEP_EVENT_FLAG_X_ACTIVE << int(axis));
        stepped = true;
    }

    // A sample without steps still queues its time, so that the queue keeps
    // time through stationary stretches rather than running dry and executing
    // the next step early.
    void sample_end(uint32_t timestamp_us) {
        if (!stepped) {
            push_at(timestamp_us, false, StepEventFlag::STEP_EVENT_FLAG_KEEP_ALIVE);
        }
        stepped = false;
        end_us = timestamp_us;
    }

    void wait_for_samples() {
        notify(Event::no_signal_samples);
    }

    bool aborted() const {
        return motion_aborted();
    }

    // Holds the motion until the end of the last sample.
    void finish() {
        push_at(end_us, false, StepEventFlag::STEP_EVENT_FLAG_KEEP_ALIVE | StepEventFlag::STEP_EVENT_FLAG_END_OF_MOTION);
        release();
    }

private:
    void release() {
        if (held) {
            start_us = held->release();
            held.reset();
            notify(Event::motion_start);
        }
    }

    void push_at(uint32_t timestamp_us, bool dir, StepEventFlag_t flags) {
        uint32_t delay_us = timestamp_us - last_us;
        while (delay_us > STEP_TIMER_MAX_TICKS_LIMIT) {
            push(STEP_TIMER_MAX_TICKS_LIMIT, false, StepEventFlag::STEP_EVENT_FLAG_KEEP_ALIVE);
            delay_us -= STEP_TIMER_MAX_TICKS_LIMIT;
        }
        push(delay_us, dir, flags);
        last_us = timestamp_us;
    }

    void push(uint32_t delay_us, bool dir, StepEventFlag_t flags) {
        while (precise_stepping::manual::is_full() && !motion_aborted()) {
            release();
            notify(Event::step_queue_full);
        }
        if (motion_aborted()) {
            return;
        }
        precise_stepping::manual::enqueue_step(delay_us, dir, flags);
    }

    F &on_event;
    std::optional<precise_stepping::manual::HeldStart> held;
    uint32_t start_us = 0;
    uint32_t last_us = 0;
    uint32_t end_us = 0;
    bool stepped = false;
};

template <sp::pipe::Source Signal, EventHandler F>
void execute(Signal native_positions, F &on_event) {
    debug_assert(!planner.processing());
    debug_assert(!phase_stepping::any_axis_enabled());

    std::array<float, STEPPER_AXES> mm_per_step;
    for (int i = 0; i < STEPPER_AXES; i++) {
        mm_per_step[i] = planner.mm_per_step[i];
    }

    auto motor_positions = std::move(native_positions)
        | sp::pipe::transform([first = true, &mm_per_step](const xyze_pos_t &native) mutable {
              const MachinePosXYZE machine = to_machine_pos(native);
              if (first) {
                  // Steps are only ever generated from here on, so a start away
                  // from where the machine is would be a jump nobody steps.
                  [[maybe_unused]] const MachinePosXYZE at = planner.get_machine_position_mm();
                  for (int i = X_AXIS; i <= Z_AXIS; i++) {
                      debug_assert(std::abs(machine[i] - at[i]) <= mm_per_step[i]);
                  }
                  first = false;
              }
              return machine_to_motor(machine);
          });

    enable_all_steppers();

    StepQueue<F> queue(on_event);
    queue.start();
    generate_steps(std::move(motor_positions), mm_per_step, queue);
    queue.finish();

    while (precise_stepping::manual::has_steps() || PreciseStepping::stopping()) {
        queue.notify(Event::flushing_steps);
    }

    planner.reset_position();
    set_current_from_steppers();
    PreciseStepping::reset_from_halt(false);
}

} // namespace detail

/// Executes a sampled trajectory, bypassing the planner.
///
/// Samples are native positions: XYZ absolute, E relative to the start of the
/// trajectory (an absolute E counter would be past float resolution for a
/// single sample's worth of filament). Bed levelling is applied to every
/// sample; to move without it, disable levelling beforehand. The step
/// resolution is the planner's; for a different microstepping, reconfigure
/// the planner beforehand.
///
/// Requires the planner idle, phase stepping disabled and the first sample at
/// the current position. Returns once the motion is over - at the end of the
/// signal, or early if the signal ends (done) or the motion is stopped - with
/// the planner and current_position, E included, taken back from the steppers.
template <EventHandler F = decltype([](Event, uint32_t) { idle(true); })>
void run_trajectory(sp::pipe::SignalSource<xyze_pos_t> native_positions, F on_event = {}) {
    detail::execute(std::move(native_positions), on_event);
}

/// Executes a sampled speed profile, bypassing the planner, starting at the
/// current position. Speeds are native, in mm/s. Otherwise as run_trajectory().
template <EventHandler F = decltype([](Event, uint32_t) { idle(true); })>
void run_speed_profile(sp::pipe::SignalSource<xyze_float_t> native_speeds, F on_event = {}) {
    const sp::SamplingFreq freq = native_speeds.sampling_freq();
    const float dt = 1.0f / freq;
    xyze_pos_t origin = current_position;
    origin.e = 0.f;
    // Each speed sample moves from one position to the next, so the start
    // comes first. Integrated from zero and offset only on output: the start
    // offset would otherwise round every increment, a bias that accumulates.
    detail::execute(
        sp::pipe::chain(
            sp::pipe::make_constant(origin, freq) | sp::pipe::take_samples(1),
            std::move(native_speeds)
                | sp::pipe::transform([origin, dt, travelled = xyze_float_t {}](const xyze_float_t &speed) mutable {
                      travelled += speed * dt;
                      return origin + travelled;
                  })),
        on_event);
}

namespace detail {

// Fuse four scalar generators into a single XYZE-like vector generator. All
// generators must have the same sampling frequency. Once one is exhausted, its
// last value is held. Output length equals the longest input.
template <typename OutType, typename Gen0, typename Gen1, typename Gen2, typename Gen3>
inline auto fuse(Gen0&& g0, Gen1&& g1, Gen2&& g2, Gen3&& g3) {
    debug_assert(
        (g0.sampling_freq() == g1.sampling_freq()) &&
        (g0.sampling_freq() == g2.sampling_freq()) &&
        (g0.sampling_freq() == g3.sampling_freq())
    );
    return sp::pipe::zip_longest_tail(
        std::forward<Gen0>(g0),
        std::forward<Gen1>(g1),
        std::forward<Gen2>(g2),
        std::forward<Gen3>(g3)
    ) | sp::pipe::transform([](const auto &t) {
        OutType r;
        r[0] = std::get<0>(t);
        r[1] = std::get<1>(t);
        r[2] = std::get<2>(t);
        r[3] = std::get<3>(t);
        return r;
    });
}

} // namespace detail

template <typename XGen, typename YGen, typename ZGen, typename EGen>
inline auto fuse_xyze(XGen&& x_gen, YGen&& y_gen, ZGen&& z_gen, EGen&& e_gen) {
    return detail::fuse<xyze_float_t>(
        std::forward<XGen>(x_gen), std::forward<YGen>(y_gen),
        std::forward<ZGen>(z_gen), std::forward<EGen>(e_gen));
}

} // namespace signal2step
