#pragma once

// This module implements a conversion of a sampled trajectory at fixed
// frequency to stepper motors steps.

#include <signal_processing/pipeline.hpp>
#include <core/types.h>
#include <core/mtypes.hpp>

#ifndef UNITTESTS
#include <feature/bedlevel/bedlevel.h>
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <bsod/bsod.h>

namespace signal2step {

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

} // namespace detail

template <typename F>
concept StepYielder = requires(F f, uint32_t timestamp, AxisEnum axis, bool direction) {
    { f(timestamp, axis, direction) } -> std::same_as<void>;
};

template <typename F>
concept WaitForSamples = requires(F f) {
    { f() } -> std::same_as<void>;
};

// Convert a signal sampled at fixed frequency to stepper motor steps. It is
// assumed that XYZE directly corresponds to the motors beeing stepped. If you
// need to convert cartesian to other kinematics, use helper pipeline nodes
// below. The steps yielded have absolute timestamps in microseconds starting at
// the beginning of the signal. Final stepped positions is returned.
template <StepYielder F, WaitForSamples Wait = decltype([]{})>
abce_pos_t convert(
    sp::pipe::SignalSource<abce_pos_t> signal,
    abce_pos_t mm_per_step,
    F yield_step, Wait wait_for_samples = {}) {

    debug_assert(signal.sampling_freq() > 0.f);

    constexpr int STEPPER_AXES = 4;
    std::array<float, STEPPER_AXES> mm_per_step_arr;
    std::array<float, STEPPER_AXES> inv_mm_step_arr;
    for (int i = 0; i < STEPPER_AXES; i++) {
        debug_assert(mm_per_step.pos[i] > 0.f);
        mm_per_step_arr[i] = mm_per_step.pos[i];
        inv_mm_step_arr[i] = 1.0f / mm_per_step.pos[i];
    }
    std::array<int32_t, STEPPER_AXES> step_position = {};
    std::array<float, STEPPER_AXES> prev_sample = {};

    const uint32_t time_per_sample_us = static_cast<uint32_t>(1'000'000.0f / signal.sampling_freq());

    bool first_sample = true;
    uint32_t current_time_us = 0;
    while (true) {
        auto poll_result = signal.poll();
        if (poll_result == sp::pipe::PollResult::done) {
            break;
        }
        if (poll_result == sp::pipe::PollResult::pending) {
            wait_for_samples();
            continue;
        }

        abce_pos_t s = signal.next();
        std::array<float, STEPPER_AXES> sample;
        for (int i = 0; i < STEPPER_AXES; i++) {
            sample[i] = s[i];
        }

        // The first sample is the starting position; no steps are generated.
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
                mm_per_step_arr[i],
                inv_mm_step_arr[i],
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
            if (axis.direction)
                step_position[axis_to_step]++;
            else
                step_position[axis_to_step]--;
            axis.advance();
            yield_step(earliest_step_time_us, detail::idx_to_axis(axis_to_step), axis.direction);
        }

        // Update state for next iteration
        prev_sample = sample;
        debug_assert(current_time_us <= std::numeric_limits<uint32_t>::max() - time_per_sample_us);
        current_time_us = next_sample_timestamp_us;
    }

    abce_pos_t result;
    for (int i = 0; i < STEPPER_AXES; i++) {
        result[i] = step_position[i] * mm_per_step[i];
    }
    return result;
}

// Pipeline helper to convert cartesian XYZE to CoreXY motor coordinates.
inline auto cartesian_to_corexy() {
    return sp::pipe::transform([](xyze_float_t v) -> abce_pos_t {
        abce_pos_t r;
        r.a = v.x + v.y;
        r.b = v.x - v.y;
        r.c = v.z;
        r.e = v.e;
        return r;
    });
}

// Pipeline helper to convert XY motion to printer kinematics
inline auto cartesian_to_printer_kinematics() {
    #ifdef COREXY
    return cartesian_to_corexy();
    #else
    return sp::pipe::transform([](xyze_float_t v) -> abce_pos_t {
        abce_pos_t r;
        r.a = v.x;
        r.b = v.y;
        r.c = v.z;
        r.e = v.e;
        return r;
    });
    #endif
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

template <typename AGen, typename BGen, typename CGen, typename EGen>
inline auto fuse_abce(AGen&& a_gen, BGen&& b_gen, CGen&& c_gen, EGen&& e_gen) {
    return detail::fuse<abce_pos_t>(
        std::forward<AGen>(a_gen), std::forward<BGen>(b_gen),
        std::forward<CGen>(c_gen), std::forward<EGen>(e_gen));
}

inline xyze_pos_t from_machine_to_cartesian(const abce_pos_t &machine_pos) {
    xyze_pos_t r;
#ifdef COREXY
    r.x = (machine_pos.a + machine_pos.b) / 2.f;
    r.y = (machine_pos.a - machine_pos.b) / 2.f;
#else
    r.x = machine_pos.a;
    r.y = machine_pos.b;
#endif
    r.z = machine_pos.c;
    r.e = machine_pos.e;
    return r;
}

} // namespace signal2step
