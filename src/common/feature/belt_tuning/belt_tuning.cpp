#include "belt_tuning.hpp"
#include "printer_belt_parameters.hpp"

#include <Marlin/src/gcode/calibrate/M958.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <logging/log.hpp>
#include <option/has_toolchanger.h>

#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

// Sanity checks for linearly increasing amplitude
static_assert([] {
    using P = MeasureBeltTensionSpecificParams;
    const P::ExcitationAmplitudeFunc f = P::linearly_varying_amplitude<75.0f, 105.0f, 0.00007f, 0.00009f>;
    return (f(75) == 0.00007f)
        && (f(90) == 0.00008f)
        && (f(105) == 0.00009f);
}());

// Check that resonant_frequency_to_tension and tension_to_resonant_frequency are inverse to each other
static_assert([] {
    const auto &bs = printer_belt_parameters.belt_system[0];
    return abs(bs.resonant_frequency_to_tension(bs.tension_to_resonant_frequency(bs.target_tension_force_n)) - bs.target_tension_force_n) < 0.01f;
}());

LOG_COMPONENT_REF(Marlin);

std::optional<MeasureBeltTensionResult> measure_belt_tension(const MeasureBeltTensionParams &config) {
    if (config.belt_system >= PrinterBeltParameters::belt_system_count) {
        return std::nullopt;
    }

    const PrinterBeltParameters::BeltSystemParameters &belt_system_params = printer_belt_parameters.belt_system[config.belt_system];

    // phstep needs to be off _before_ getting the current ustep resolution
    phase_stepping::EnsureDisabled phaseSteppingDisabler;

    if (!config.skip_setup) {
        const auto aborted = [&] {
            return config.progress_callback && !config.progress_callback({});
        };

        // Make sure we're homed
        if (!GcodeSuite::G28_no_parser(true, 5, false, true, true, true)) {
            return std::nullopt;
        }

        if (aborted()) {
            return std::nullopt;
        }

#if HAS_TOOLCHANGER()
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            // Always do the measurements with the first tool for consistency
            tool_change(0, tool_return_t::no_return);

            if (aborted()) {
                return std::nullopt;
            }
        }
#endif

        // Move to the position where we shall measure
        do_blocking_move_to(belt_system_params.measurement_pos);

        if (aborted()) {
            return std::nullopt;
        }
    }

    if (config.skip_tuning) {
        // Return something, to discern from failure to home or such
        return MeasureBeltTensionResult {};
    }

    MicrostepRestorer microstep_restorer;

    // Taken from M958::setup_axis
    stepper_microsteps(X_AXIS, 128);
    stepper_microsteps(Y_AXIS, 128);

    static constexpr std::array<StepEventFlag_t, PrinterBeltParameters::belt_system_count> belt_system_axis_flags {
#if ENABLED(COREXY)
        // Vibrate the toolhead front and back
        STEP_EVENT_FLAG_STEP_X | STEP_EVENT_FLAG_STEP_Y | STEP_EVENT_FLAG_Y_DIR,
#else
        STEP_EVENT_FLAG_STEP_X,
            STEP_EVENT_FLAG_STEP_Y,
#endif
    };

    VibrateMeasureParams measure_params {
        .excitation_amplitude = config.excitation_amplitude_m,
        .excitation_cycles = config.excitation_cycles,
        .wait_cycles = config.wait_cycles,
        .measurement_cycles = config.measurement_cycles,
        .klipper_mode = false,
        .calibrate_accelerometer = config.calibrate_accelerometer,
        .axis_flag = belt_system_axis_flags[config.belt_system],
        .measured_harmonic = config.measured_harmonic,
    };
    if (!measure_params.setup(microstep_restorer)) {
        return std::nullopt;
    }

    struct MeasureRecord {
        float frequency;
        float amplitude;
    };
    std::optional<MeasureRecord> best_match;

    MeasureBeltTensionParams::ProgressCallbackArgs progress_args;

    // Sweep the frequency range, find the most resonant frequency
    for (float frequency = config.start_frequency_hz; frequency <= config.end_frequency_hz; frequency += config.frequency_step_hz) {
        if (auto f = config.excitation_amplitude_m_func) {
            measure_params.excitation_amplitude = f(frequency);
        }

        const auto measure_result = vibrate_measure_repeat(measure_params, frequency, [&](auto) {
            idle(true, true);

            return !config.progress_callback || config.progress_callback(progress_args);
        });

        if (!measure_result.has_value()) {
            return std::nullopt;
        }

        measure_params.calibrate_accelerometer = false;

        const float amplitude = measure_result->amplitude[X_AXIS] * measure_result->amplitude[Y_AXIS];
        log_info(Marlin, "Belt tuning: %f Hz -> %f", (double)measure_result->excitation_frequency, (double)amplitude);

        if (!best_match || best_match->amplitude < amplitude) {
            best_match = MeasureRecord {
                .frequency = measure_result->excitation_frequency,
                .amplitude = amplitude,
            };
        }

        progress_args = {
            .overall_progress = (frequency - config.start_frequency_hz) / (config.end_frequency_hz - config.start_frequency_hz),
            .last_frequency = frequency,
            .last_result = amplitude,
        };
        if (config.progress_callback && !config.progress_callback(progress_args)) {
            return std::nullopt;
        }
    }

    if (!best_match) {
        return std::nullopt;
    }

    {
        const MeasureBeltTensionResult result {
            .belt_system = config.belt_system,
            .resonant_frequency_hz = best_match->frequency,
        };

        log_info(Marlin, "Belt tuning result: %f Hz %f N", (double)result.resonant_frequency_hz, (double)result.tension_force_n());

        return result;
    }
}

float MeasureBeltTensionResult::tension_force_n() const {
    return printer_belt_parameters.belt_system[belt_system].resonant_frequency_to_tension(resonant_frequency_hz);
}
