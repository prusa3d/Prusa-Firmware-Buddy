#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include <Marlin/src/gcode/calibrate/M958.hpp>
#include <inplace_function.hpp>

/// Finds the resonances of the belt spans along the X gantry with the head
/// accelerometer.
///
/// Vibrated as hard as the belt tuning needs for the belts to be seen, a belt
/// span behaves as a hardening resonator. Sweeping the frequency up, its
/// response grows and then collapses at once. Sweeping down, it jumps up
/// at a lower frequency. In between, the belt keeps whichever of the two
/// states it was in. The lower end of that range is close to the frequency
/// the belt has when it barely moves, which is what its tension determines.
///
/// A span resonance rises as the span gets shorter, so it is told apart from
/// the resonances of the rest of the printer by measuring at two head
/// positions.
namespace belt_resonance {

inline constexpr float sweep_start_frequency = 80.f; ///< [Hz]
inline constexpr float sweep_frequency_step = 0.5f; ///< [Hz]
inline constexpr size_t sweep_size = 97; ///< Up to 128 Hz

/// Amplitude of the head acceleration at each frequency of a sweep [m/s^2]
using Sweep = std::array<float, sweep_size>;

constexpr float sweep_frequency(size_t index) {
    return sweep_start_frequency + index * sweep_frequency_step;
}

/// Called repeatedly during a sweep with its progress (0-1).
/// \returns false to abort the sweep
using SweepHook = stdext::inplace_function<bool(float progress), 24>;

/// Measures the accelerometer sample rate, takes over ten seconds.
/// \returns the rate [Hz], nullopt on failure
std::optional<float> measure_sample_rate(const SweepHook &hook);

/// Vibrates the head at the current position through the sweep frequencies,
/// with the amplitude of the belt tuning, and records the response.
/// Needs phase stepping disabled and a tool picked.
/// \returns false if aborted or failed
bool measure_sweep(Sweep &sweep, bool downwards, StepEventFlag_t axis_flag,
    const vibrate_measure::MicrostepRestorer &microstep_restorer, float sample_rate, const SweepHook &hook);

struct Reading {
    /// Lower belt: where its response jumps up in a sweep down [Hz]
    float lower_belt;

    /// Lower belt: where its response collapses in a sweep up [Hz]
    float lower_belt_collapse;

    /// Upper belt: where its response collapses in a sweep up [Hz].
    /// Missing when no collapse of its own was found, the two belts may
    /// collapse together then.
    std::optional<float> upper_belt_collapse;

    /// The jump of the lower belt was not found, most likely hidden in the
    /// response of the frame because the belt is loose. \p lower_belt is
    /// only estimated from where the belt collapses.
    bool coarse = false;
};

/// Finds the belts in sweeps taken at the calibration position (down and up)
/// and with the head moved towards the far end of the measured spans (down).
std::optional<Reading> evaluate(const Sweep &down, const Sweep &up, const Sweep &down_far);

} // namespace belt_resonance
