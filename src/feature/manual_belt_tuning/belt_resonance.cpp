#include "belt_resonance.hpp"
#include "manual_belt_tuning_config.hpp"

#include <Marlin/src/Marlin.h>
#include <Marlin/src/module/planner.h>
#include <Marlin/src/module/prusa/accelerometer.h>
#include <feature/phase_stepping/phase_stepping.hpp>
#include <feature/precise_stepping/manual.hpp>

#include <accelerometer/common_structs.hpp>
#include <logging/log.hpp>
#include <timing.h>

#include <algorithm>
#include <cmath>
#include <numbers>

LOG_COMPONENT_DEF(BeltTuning, logging::Severity::info);

using namespace manual_belt_tuning;
using namespace vibrate_measure;
using accelerometer::RawAcceleration;

namespace belt_resonance {

namespace {

    /// Vibration periods to wait at each frequency before measuring, and to measure for
    constexpr uint32_t settle_cycles = 6;
    constexpr uint32_t measure_cycles = 16;

    /// How far ahead the vibration is kept queued, when the queue can hold
    /// that much. The queue cannot be kept full instead: with small amplitudes
    /// it holds seconds of motion, which would delay every frequency change by
    /// as much.
    constexpr uint32_t excitation_lead_us = 120'000;
    constexpr uint32_t excitation_lead_min_us = 20'000;

    /// Samples may be this old when they get here
    constexpr uint32_t sample_latency_us = 50'000;

    constexpr int point_attempts = 20;

    /// Longest time one attempt at one frequency may take, a silent accelerometer must not hold the sweep forever
    constexpr int32_t point_timeout_ms = 3000;

    /// A change of the response between neighbouring frequencies that counts
    /// as a jump, and one that still belongs to a jump already found
    constexpr float jump_ratio = 0.2f;
    constexpr float jump_continuation_ratio = 0.1f;

    /// How much the resonance of a measured span rises between the two
    /// measurement positions. Wide, the frequencies of the jumps are not
    /// exact and depend on the amplitude as well.
    constexpr float span_ratio_min = 1.03f;
    constexpr float span_ratio_max = 1.10f;

    /// Below this, the jumps of the belts cannot be told from the response of the frame [Hz]
    constexpr float belt_frequency_min = 84.f;

    /// Where the collapse of the lower belt is looked for above its jump, and
    /// the collapse of the upper belt above that one [Hz]
    constexpr float lower_collapse_distance_min = 3.f;
    constexpr float lower_collapse_distance_max = 14.f;
    constexpr float upper_collapse_distance_min = 2.f;
    constexpr float upper_collapse_distance_max = 12.f;

    /// Where the collapse of a lower belt too loose to show its jump is looked for [Hz]
    constexpr float loose_collapse_min = 88.f;
    constexpr float loose_collapse_max = belt_frequency_min + lower_collapse_distance_max;

    constexpr float angular_frequency(float frequency) {
        return 2 * std::numbers::pi_v<float> * frequency;
    }

    /// Amplitude of the belt tuning vibration at the given frequency [steps]
    unsigned amplitude_steps(float frequency, float step_len) {
        const float acceleration = calc_accel(frequency) * 0.001f;
        return static_cast<unsigned>(std::ceil(acceleration / (angular_frequency(frequency) * angular_frequency(frequency)) / step_len));
    }

    /// Keeps the steppers vibrating, a bit of motion ahead
    class Exciter {
    public:
        Exciter(Vibrate &vibrator)
            : vibrator(vibrator) {}

        /// To be called often.
        /// \returns false if the vibration got interrupted since the last call
        bool feed() {
            const uint32_t now = ticks_us();
            bool continuous = true;
            if (!precise_stepping::manual::has_steps()) {
                continuous = !started;
                queued_until_us = now;
                started = true;
            }
            while (ticks_diff(queued_until_us, now) < static_cast<int32_t>(lead_us)) {
                vibrator.step();
                queued_until_us += static_cast<uint32_t>(std::lround(1'000'000.f / vibrator.frequency));
            }
            return continuous;
        }

        /// Adjusts how far ahead to queue to the vibration set in the
        /// vibrator. Queueing more than fits would block until it does,
        /// for too long to keep up with the accelerometer.
        void set_amplitude(unsigned steps) {
            const float period_us = 1'000'000.f / vibrator.frequency;
            const float queue_periods = static_cast<float>(STEP_EVENT_QUEUE_SIZE) / (4 * steps);
            const float fitting_us = (queue_periods - 1.5f) * period_us;
            lead_us = static_cast<uint32_t>(std::clamp<float>(fitting_us, excitation_lead_min_us, excitation_lead_us));
        }

        /// When the currently set vibration parameters start to apply
        uint32_t queued_until() const {
            return queued_until_us;
        }

    private:
        Vibrate &vibrator;
        uint32_t queued_until_us = 0;
        uint32_t lead_us = excitation_lead_min_us;
        bool started = false;
    };

    /// Correlates the accelerometer samples with a sine and a cosine of one frequency
    class LockIn {
    public:
        LockIn(float frequency, float sample_period)
            : phase_step(angular_frequency(frequency) * sample_period) {}

        void add(const RawAcceleration &sample) {
            const float sine = std::sin(phase);
            const float cosine = std::cos(phase);
            phase += phase_step;
            if (phase > 2 * std::numbers::pi_v<float>) {
                phase -= 2 * std::numbers::pi_v<float>;
            }

            sine_sum += sine;
            cosine_sum += cosine;
            for (size_t axis = 0; axis < 3; ++axis) {
                const float value = accelerometer::raw_to_accel(sample.val[axis]);
                sum[axis] += value;
                sine_product[axis] += value * sine;
                cosine_product[axis] += value * cosine;
            }
            ++count;
        }

        uint32_t samples() const {
            return count;
        }

        /// Amplitude of the acceleration at the frequency, all axes together [m/s^2]
        float amplitude() const {
            float square_sum = 0;
            for (size_t axis = 0; axis < 3; ++axis) {
                // Without the constant part of the signal, gravity mostly
                const float mean = sum[axis] / count;
                const float sine_part = 2 * (sine_product[axis] - mean * sine_sum) / count;
                const float cosine_part = 2 * (cosine_product[axis] - mean * cosine_sum) / count;
                square_sum += sine_part * sine_part + cosine_part * cosine_part;
            }
            return std::sqrt(square_sum);
        }

    private:
        float phase_step;
        float phase = 0;
        float sine_sum = 0;
        float cosine_sum = 0;
        float sum[3] = {};
        float sine_product[3] = {};
        float cosine_product[3] = {};
        uint32_t count = 0;
    };

    struct Jump {
        float frequency; ///< [Hz]
        float size; ///< Change of the response [m/s^2]
    };

    /// More jumps than this in one sweep do not happen in practice
    using Jumps = std::array<Jump, 16>;

    /// Finds where the response collapses going up in frequency (\p downwards false)
    /// or jumps up going down in frequency (\p downwards true).
    /// \returns the number of jumps found, ordered by frequency
    size_t find_jumps(const Sweep &sweep, bool downwards, Jumps &jumps) {
        size_t count = 0;
        // Walks in the direction the sweep was measured in
        for (size_t n = 0; n + 1 < sweep_size && count < jumps.size();) {
            const auto at = [&](size_t step) {
                return downwards ? sweep[sweep_size - 1 - step] : sweep[step];
            };
            // Up: the response drops. Down: the response rises.
            const auto changed = [&](size_t step, float ratio) {
                return downwards ? at(step + 1) > (1 + ratio) * at(step) : at(step + 1) < (1 - ratio) * at(step);
            };
            if (!changed(n, jump_ratio)) {
                ++n;
                continue;
            }

            size_t end = n + 1;
            while (end + 1 < sweep_size && changed(end, jump_continuation_ratio)) {
                ++end;
            }
            const size_t index = downwards ? sweep_size - 1 - n : n;
            jumps[count++] = {
                .frequency = sweep_frequency(index) + (downwards ? -1 : 1) * sweep_frequency_step / 2,
                .size = std::abs(at(end) - at(n)),
            };
            n = end;
        }
        std::sort(jumps.begin(), jumps.begin() + count, [](const Jump &a, const Jump &b) { return a.frequency < b.frequency; });
        return count;
    }

} // namespace

std::optional<float> measure_sample_rate(const SweepHook &hook) {
    PrusaAccelerometer accelerometer;
    if (const char *error = accelerometer.error_str()) {
        log_error(BeltTuning, "accelerometer: %s", error);
        return std::nullopt;
    }

    const auto sample_period = maybe_calibrate_and_get_accelerometer_sample_period(
        accelerometer, true,
        [&hook](float progress) -> Result<void> {
            if (!hook(progress)) {
                return std::unexpected(Error::aborted);
            }
            idle(true);
            return {};
        });
    if (!sample_period.has_value()) {
        return std::nullopt;
    }
    return 1.f / sample_period.value();
}

bool measure_sweep(Sweep &sweep, bool downwards, StepEventFlag_t axis_flag,
    const MicrostepRestorer &microstep_restorer, float sample_rate, const SweepHook &hook) {
    // The vibrations are stepped directly
    phase_stepping::assert_disabled();

    Vibrate vibrator {
        .frequency = sweep_start_frequency,
        .excitation_acceleration = 0,
        .axis_flag = axis_flag,
    };
    if (!vibrator.setup(microstep_restorer)) {
        return false;
    }

    PrusaAccelerometer accelerometer;
    if (const char *error = accelerometer.error_str()) {
        log_error(BeltTuning, "accelerometer: %s", error);
        return false;
    }

    const auto wait_for_steps = [] {
        while (precise_stepping::manual::has_steps()) {
            idle(true);
        }
    };

    Exciter exciter(vibrator);
    for (size_t n = 0; n < sweep_size; ++n) {
        const size_t index = downwards ? sweep_size - 1 - n : n;
        const float frequency = sweep_frequency(index);
        const unsigned steps = amplitude_steps(frequency, vibrator.step_len);

        vibrator.frequency = frequency;
        // The generator rounds the amplitude up to whole steps
        vibrator.excitation_acceleration = (steps - 0.5f) * vibrator.step_len * angular_frequency(frequency) * angular_frequency(frequency);
        exciter.set_amplitude(steps);

        const uint32_t settle_us = static_cast<uint32_t>(settle_cycles * 1'000'000.f / frequency) + sample_latency_us;
        const uint32_t samples_needed = static_cast<uint32_t>(std::lround(measure_cycles / frequency * sample_rate));

        for (int attempt = 0;; ++attempt) {
            if (attempt >= point_attempts) {
                log_error(BeltTuning, "sweep failed at %.1f Hz", static_cast<double>(frequency));
                wait_for_steps();
                return false;
            }

            exciter.feed();
            const uint32_t measure_from_us = exciter.queued_until() + settle_us;
            const uint32_t attempt_start_ms = ticks_ms();
            LockIn lock_in(frequency, 1.f / sample_rate);
            bool valid = true;

            while (valid && lock_in.samples() < samples_needed) {
                if (planner.draining() || !hook(static_cast<float>(n) / sweep_size)) {
                    wait_for_steps();
                    return false;
                }
                if (ticks_diff(ticks_ms(), attempt_start_ms) > point_timeout_ms) {
                    break;
                }
                // An interrupted vibration lets the belts calm down, start over at this frequency
                valid = exciter.feed();

                const bool measuring = ticks_diff(ticks_us(), measure_from_us) >= 0;
                bool empty = false;
                while (valid && !empty && lock_in.samples() < samples_needed) {
                    RawAcceleration sample;
                    switch (accelerometer.get_sample(sample)) {

                    case PrusaAccelerometer::GetSampleResult::ok:
                        if (measuring) {
                            lock_in.add(sample);
                        }
                        break;

                    case PrusaAccelerometer::GetSampleResult::buffer_empty:
                        empty = true;
                        break;

                    case PrusaAccelerometer::GetSampleResult::error:
                        accelerometer.clear();
                        valid = false;
                        break;
                    }
                }
                if (empty) {
                    idle(true);
                }
            }

            if (valid && lock_in.samples() >= samples_needed) {
                sweep[index] = lock_in.amplitude();
                break;
            }
        }
    }

    wait_for_steps();
    return true;
}

std::optional<Reading> evaluate(const Sweep &down, const Sweep &up, const Sweep &down_far) {
    Jumps jumps, jumps_far, collapses;
    const size_t jump_count = find_jumps(down, true, jumps);
    const size_t jump_far_count = find_jumps(down_far, true, jumps_far);
    const size_t collapse_count = find_jumps(up, false, collapses);

    // The lower belt makes the largest of the jumps that move with the head as a belt span does
    const Jump *lower = nullptr;
    for (size_t i = 0; i < jump_count; ++i) {
        const Jump &jump = jumps[i];
        if (jump.frequency < belt_frequency_min) {
            continue;
        }
        const bool moves = std::any_of(jumps_far.begin(), jumps_far.begin() + jump_far_count, [&](const Jump &far) {
            const float ratio = far.frequency / jump.frequency;
            return ratio >= span_ratio_min && ratio <= span_ratio_max;
        });
        if (moves && (!lower || jump.size > lower->size)) {
            lower = &jump;
        }
    }
    const auto first_collapse = [&](float from, float to) -> std::optional<float> {
        for (size_t i = 0; i < collapse_count; ++i) {
            if (collapses[i].frequency >= from && collapses[i].frequency <= to) {
                return collapses[i].frequency;
            }
        }
        return std::nullopt;
    };

    Reading reading;
    if (lower) {
        const auto lower_collapse = first_collapse(lower->frequency + lower_collapse_distance_min, lower->frequency + lower_collapse_distance_max);
        if (!lower_collapse) {
            log_info(BeltTuning, "lower belt %.2f Hz without a collapse", static_cast<double>(lower->frequency));
            return std::nullopt;
        }
        reading.lower_belt = lower->frequency;
        reading.lower_belt_collapse = *lower_collapse;

    } else {
        // A loose lower belt jumps within the response of the frame, where it cannot be told. It still
        // collapses above that, as the largest thing there; its jump is then about the usual distance below.
        const Jump *collapse = nullptr;
        for (size_t i = 0; i < collapse_count; ++i) {
            const Jump &candidate = collapses[i];
            if (candidate.frequency >= loose_collapse_min && candidate.frequency <= loose_collapse_max && (!collapse || candidate.size > collapse->size)) {
                collapse = &candidate;
            }
        }
        if (!collapse) {
            log_info(BeltTuning, "no belt found, jumps %u and %u, collapses %u",
                static_cast<unsigned>(jump_count), static_cast<unsigned>(jump_far_count), static_cast<unsigned>(collapse_count));
            return std::nullopt;
        }
        reading.lower_belt = collapse->frequency - automatic_collapse_distance;
        reading.lower_belt_collapse = collapse->frequency;
        reading.coarse = true;
    }

    reading.upper_belt_collapse = first_collapse(reading.lower_belt_collapse + upper_collapse_distance_min, reading.lower_belt_collapse + upper_collapse_distance_max);
    log_info(BeltTuning, "lower belt %.2f Hz%s, collapses at %.2f Hz, upper belt collapses at %.2f Hz",
        static_cast<double>(reading.lower_belt), reading.coarse ? " (estimated)" : "", static_cast<double>(reading.lower_belt_collapse),
        static_cast<double>(reading.upper_belt_collapse.value_or(0)));
    return reading;
}

} // namespace belt_resonance
