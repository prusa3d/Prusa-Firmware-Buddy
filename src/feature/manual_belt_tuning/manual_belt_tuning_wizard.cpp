#include "manual_belt_tuning_wizard.hpp"
#include "manual_belt_tuning_config.hpp"

#include <Marlin/src/Marlin.h>
#include <Marlin/src/gcode/calibrate/M958.hpp>
#include <feature/motordriver_util.h>
#include <feature/phase_stepping/phase_stepping.hpp>

#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

#include <option/has_automatic_belt_tuning.h>
#if HAS_AUTOMATIC_BELT_TUNING()
    #include "belt_resonance.hpp"
    #include <feature/precise_stepping/manual.hpp>
    #include <logging/log.hpp>
    #include <mapi/calibration_preamble.hpp>
    #include <tool_index.hpp>
    #include <array>
    #include <memory>
#endif

#include <option/has_gantry_squareness_check.h>
#if HAS_GANTRY_SQUARENESS_CHECK()
    #include <feature/indx_gantry_squareness/indx_gantry_squareness.hpp>
#endif

#include <fsm/manual_belt_tuning_phases.hpp>
#include <common/marlin_server.hpp>
#include <bsod/bsod.h>
#include <string_builder.hpp>
#include <selftest/selftest_invocation.hpp>

#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/motion.h>

#include <cmath>
#include <optional>

#if HAS_AUTOMATIC_BELT_TUNING()
LOG_COMPONENT_REF(BeltTuning);
#endif

using namespace marlin_server;
using namespace manual_belt_tuning;
using namespace vibrate_measure;

namespace {

class ManualBeltTuningWizard {
private:
    enum class Result {
        abort,
        retry,
        pass,
    };

    phase_stepping::EnsureDisabled phase_stepping_disabler;

public:
    ManualBeltTuningWizard() {}
    ~ManualBeltTuningWizard() {
#if HAS_XBUDDY_EXTENSION()
        buddy::xbuddy_extension().set_strobe(std::nullopt);
#else
    #error "This is a vital part of the calibration, without LED stroboscop this calibration is not reasonable (user won't see the correct resonating frequency)"
#endif
        disable_all_steppers();
    }

#if HAS_AUTOMATIC_BELT_TUNING()
    enum class Outcome {
        ok,
        failed,
        aborted,
        no_accelerometer,
    };

    /// Stays the same for the whole run, measuring it takes long
    std::optional<float> sample_rate;

    /// Cleared when the accelerometer turns out not to be usable, the rest of the run is manual then
    bool accelerometer_available = true;

    /// Records how the head responds to the vibration at the current position
    bool sweep_here(belt_resonance::Sweep &sweep, bool downwards, const belt_resonance::SweepHook &hook) {
        // The planner needs the original microstepping back for the next move
        MicrostepRestorer microstep_restorer;
        enable_all_steppers();
        // Front and back, across the measured belt spans
        const StepEventFlag_t axis_flag = setup_steppers(STEP_EVENT_FLAG_STEP_Y);
        return belt_resonance::measure_sweep(sweep, downwards, axis_flag, microstep_restorer, *sample_rate, hook);
    }

    Outcome measure_belts(belt_resonance::Reading &reading) {
        static constexpr auto phase = PhaseManualBeltTuning::measuring;

        // The sample rate measurement and the three sweeps take about the same time
        const float parts = sample_rate ? 3 : 4;
        float parts_done = 0;
        int shown_percent = -1;
        bool aborted = false;
        const auto hook = [&](float progress) {
            const int percent = static_cast<int>((parts_done + progress) * 100 / parts);
            if (percent != shown_percent) {
                shown_percent = percent;
                fsm_change(phase, fsm::PhaseData { static_cast<uint8_t>(percent) });
            }
            if (get_response_from_phase(phase) == Response::Abort) {
                aborted = true;
            }
            return !aborted;
        };
        hook(0);

        if (!sample_rate) {
            sample_rate = belt_resonance::measure_sample_rate(hook);
            if (!sample_rate) {
                return aborted ? Outcome::aborted : Outcome::no_accelerometer;
            }
            ++parts_done;
        }

        const auto move_to_x = [](float x) {
            do_blocking_move_to_xy(x, calib_position_y, 50.f);
            planner.synchronize();
        };

        // Up, down, and down again with the head further along the measured spans
        const auto sweeps = std::make_unique<std::array<belt_resonance::Sweep, 3>>();
        const float xs[] = { calib_position_x, calib_position_x, calib_position_x + automatic_second_position_x_offset };
        const bool downwards[] = { false, true, true };
        bool ok = true;
        for (size_t i = 0; ok && i < sweeps->size(); ++i) {
            move_to_x(xs[i]);
            ok = sweep_here((*sweeps)[i], downwards[i], hook);
            ++parts_done;
        }
        move_to_x(calib_position_x);
        if (!ok) {
            return aborted ? Outcome::aborted : Outcome::failed;
        }

        const auto result = belt_resonance::evaluate((*sweeps)[1], (*sweeps)[0], (*sweeps)[2]);
        if (!result) {
            return Outcome::failed;
        }
        reading = *result;
        return Outcome::ok;
    }

    /// Vibrates through the frequencies from the current one of the \p vibrator
    /// to \p target and stays there, until the user responds to the \p phase.
    /// The belts end up in the state they have in a sweep up.
    Response ramp_and_hold(Vibrate &vibrator, float target, PhaseManualBeltTuning phase) {
        // Slow enough for the belts to follow
        static constexpr float ramp_step = 0.5f; // Hz
        static constexpr float ramp_step_duration = 0.3f; // s

        while (true) {
            adjust_vibrator(vibrator);
            const bool ramping = vibrator.frequency < target;
            const int periods = ramping ? static_cast<int>(vibrator.frequency * ramp_step_duration) : 1;
            for (int i = 0; i < periods; ++i) {
                vibrator.step();
                if (const Response response = get_response_from_phase(phase); response != Response::_none) {
                    return response;
                }
                idle(true);
            }
            if (ramping) {
                vibrator.frequency = std::min(vibrator.frequency + ramp_step, target);
            }
        }
    }

    /// Shows the user the upper belt around the frequency found for it.
    /// If it does not behave as expected there, the user finds its frequency.
    /// \param lower_belt frequency of the lower belt
    /// \param upper_belt_collapse where the response of the upper belt collapses, updated with what the user found
    Outcome confirm_upper_belt(float lower_belt, float &upper_belt_collapse) {
        // How far from the collapse the belt is shown, on each side
        static constexpr float margin_below = 1.f; // Hz
        static constexpr float margin_above = 2.f; // Hz
        // Started well below both belts, to get to their vibrating state on the way up
        static constexpr float start_below_lower_belt = 4.f; // Hz

        MicrostepRestorer microstep_restorer;
        enable_all_steppers();
        Vibrate vibrator {
            .frequency = lower_belt - start_below_lower_belt,
            .excitation_acceleration = 2.5f,
            .axis_flag = setup_steppers(STEP_EVENT_FLAG_STEP_Y),
        };
        if (!vibrator.setup(microstep_restorer)) {
            return Outcome::failed;
        }

        Outcome outcome = Outcome::ok;
        fsm_change(PhaseManualBeltTuning::confirm_upper_belt_wave);
        if (ramp_and_hold(vibrator, upper_belt_collapse - margin_below, PhaseManualBeltTuning::confirm_upper_belt_wave) == Response::Abort) {
            outcome = Outcome::aborted;
        } else {
            fsm_change(PhaseManualBeltTuning::confirm_upper_belt_collapse);
            switch (ramp_and_hold(vibrator, upper_belt_collapse + margin_above, PhaseManualBeltTuning::confirm_upper_belt_collapse)) {
            case Response::Yes:
                break;
            case Response::No: {
                vibrator.frequency = upper_belt_collapse;
                fsm_change(PhaseManualBeltTuning::measure_upper_belt, fsm::serialize_data<belt_tensions>({ vibrator.frequency, vibrator.frequency }));
                if (resonate(vibrator, PhaseManualBeltTuning::measure_upper_belt) == Response::Abort) {
                    outcome = Outcome::aborted;
                } else {
                    upper_belt_collapse = vibrator.frequency;
                }
                break;
            }
            default:
                outcome = Outcome::aborted;
                break;
            }
        }

        buddy::xbuddy_extension().set_strobe(std::nullopt);
        while (precise_stepping::manual::has_steps()) {
            idle(true);
        }
        return outcome;
    }

    static int8_t turn_eights(float revolutions) {
        const int eights = static_cast<int>(std::lround(revolutions * 8));
        return static_cast<int8_t>(std::clamp<int>(eights, -automatic_max_turn_eights, automatic_max_turn_eights));
    }

    /// Measures the belts with the accelerometer.
    /// \returns nullopt if the accelerometer cannot be used
    std::optional<Result> run_automatic() {
        fsm_change(PhaseManualBeltTuning::homing_wait);
        enable_all_steppers();

        // The accelerometer is not available without a tool
        const mapi::CalibrationPreamble preamble {
            .tool_policy = mapi::CalibrationPreamble::ToolPolicy::ensure_picked,
            .on_step = [](mapi::CalibrationPreamble::Step) {},
        };
        if (!preamble.run() || !PhysicalToolIndex::currently_selected_opt().has_value()) {
            return std::nullopt;
        }

        int adjustments = 0;
        while (true) {
            belt_resonance::Reading reading;
            switch (measure_belts(reading)) {
            case Outcome::ok:
                break;
            case Outcome::failed:
                fsm_change(PhaseManualBeltTuning::measurement_failed);
                switch (wait_for_response(PhaseManualBeltTuning::measurement_failed)) {
                case Response::Retry:
                    continue;
                case Response::Skip:
                    // The user finds the belts by eye instead
                    return std::nullopt;
                default:
                    return Result::abort;
                }
            case Outcome::aborted:
                return Result::abort;
            case Outcome::no_accelerometer:
                return std::nullopt;
            }

            // Without a collapse of its own, the upper belt most likely collapsed together with the lower one
            float upper_belt_collapse = reading.upper_belt_collapse.value_or(reading.lower_belt_collapse);
            // An estimate only tells which way to turn the screws, the belts get measured properly once they are closer
            if (!reading.coarse && confirm_upper_belt(reading.lower_belt, upper_belt_collapse) != Outcome::ok) {
                return Result::abort;
            }

            // The optimal frequencies are those of belts that barely move. The jump of the lower belt
            // is close to that. The upper belt only shows its collapse, which is higher, so it is
            // placed as far above the lower belt as its collapse is above the collapse of the lower belt.
            float belt_distance = upper_belt_collapse - reading.lower_belt_collapse;
            if (reading.coarse && !reading.upper_belt_collapse) {
                // A loose upper belt can collapse too weakly to be found, it is taken to be as loose as the lower one
                belt_distance = higher_freq_belt_optimal - lower_freq_belt_optimal;
            }
            const float freq_bottom_belt = reading.lower_belt;
            const float freq_top_belt = freq_bottom_belt + belt_distance;
            fsm_change(PhaseManualBeltTuning::show_measured, fsm::serialize_data<belt_tensions>({ freq_top_belt, freq_bottom_belt }));
            if (!wait_for_continue(PhaseManualBeltTuning::show_measured)) {
                return Result::abort;
            }

            const float top_error = higher_freq_belt_optimal - freq_top_belt;
            const float bottom_error = lower_freq_belt_optimal - freq_bottom_belt;
            if (std::abs(top_error) <= automatic_freq_tolerance && std::abs(bottom_error) <= automatic_freq_tolerance) {
                break;
            }

            // The measurement or the constants may not fit this printer, do not keep the user adjusting forever
            if (adjustments >= automatic_adjustment_rounds) {
                fsm_change(PhaseManualBeltTuning::not_converging);
                const Response response = wait_for_response(PhaseManualBeltTuning::not_converging);
                if (response == Response::Ignore) {
                    break;
                } else if (response != Response::Retry) {
                    return Result::abort;
                }
                adjustments = 0;
            }
            ++adjustments;

            // The left screw acts mostly on the upper belt, the right one on the lower belt
            screw_adjustment adjustment {
                .left_eights = turn_eights(calc_revs_from_freq(top_error, bottom_error, automatic_hz_per_rev, automatic_hz_per_rev2)),
                .right_eights = turn_eights(calc_revs_from_freq(bottom_error, top_error, automatic_hz_per_rev, automatic_hz_per_rev2)),
            };
            if (adjustment.left_eights == 0 && adjustment.right_eights == 0) {
                // Too little to round to a turn, yet out of the tolerance: nudge the belt that is further off
                if (std::abs(top_error) > std::abs(bottom_error)) {
                    adjustment.left_eights = top_error > 0 ? 1 : -1;
                } else {
                    adjustment.right_eights = bottom_error > 0 ? 1 : -1;
                }
            }
            log_info(BeltTuning, "upper belt %.2f Hz, lower belt %.2f Hz, turn left %d/8, right %d/8",
                static_cast<double>(freq_top_belt), static_cast<double>(freq_bottom_belt), adjustment.left_eights, adjustment.right_eights);
            fsm_change(PhaseManualBeltTuning::adjust_screws, fsm::serialize_data<screw_adjustment>(adjustment));
            if (!wait_for_continue(PhaseManualBeltTuning::adjust_screws)) {
                return Result::abort;
            }
        }

    #if HAS_GANTRY_SQUARENESS_CHECK()
        // The tensioner adjustment can move the belts on the pulleys - verify
        // again, with the squareness wizard FSM popping up over this one
        switch (indx_gantry_squareness::run_wizard()) {
        case indx_gantry_squareness::WizardResult::passed:
            break;
        case indx_gantry_squareness::WizardResult::realigned:
            return Result::retry;
        case indx_gantry_squareness::WizardResult::aborted:
            return Result::abort;
        }
    #endif

        fsm_change(PhaseManualBeltTuning::finished);
        return Result::pass;
    }
#endif

    /// The user finds the resonance of each belt by eye
    Result run_manual() {
#if !HAS_GANTRY_SQUARENESS_CHECK()
        // With the squareness check, the gantry has already been measured (selftest dependency)
        disable_all_steppers();
        fsm_change(PhaseManualBeltTuning::check_x_gantry);
        if (!wait_for_continue(PhaseManualBeltTuning::check_x_gantry)) {
            return Result::abort;
        }
#endif

        fsm_change(PhaseManualBeltTuning::homing_wait);
        enable_all_steppers();
        if (!GcodeSuite::G28_no_parser(true, true, false, { .precise = false })) {
            return Result::abort;
        }
        do_blocking_move_to_xy(calib_position_x, calib_position_y, 50.f); // belt tension calibration position
        planner.synchronize();

        fsm_change(PhaseManualBeltTuning::intro_measure);
        if (wait_for_continue(PhaseManualBeltTuning::intro_measure)) {
            const struct belt_tensions tension_data(higher_freq_belt_optimal, lower_freq_belt_optimal);
            fsm_change(PhaseManualBeltTuning::measure_upper_belt, fsm::serialize_data<belt_tensions>(tension_data));
        } else {
            return Result::abort;
        }

        // In optional so the microsteps can be restored before the final squareness check
        std::optional<MicrostepRestorer> microstep_restorer(std::in_place);

        // Taken from M958::setup_axis
        // enable all axes to have the same state as printing
        enable_all_steppers();
        stepper_microsteps(X_AXIS, 128);
        stepper_microsteps(Y_AXIS, 128);

        Vibrate vibrator {
            .frequency = 80,
            .excitation_acceleration = 2.5f,
            .axis_flag = STEP_EVENT_FLAG_STEP_X | STEP_EVENT_FLAG_STEP_Y | STEP_EVENT_FLAG_Y_DIR, // Vibrate the toolhead front and back
        };

        if (!vibrator.setup(*microstep_restorer)) {
            return Result::abort;
        }

        // Variables for result frequency
        float freq_top_belt = higher_freq_belt_optimal;
        float freq_bottom_belt = lower_freq_belt_optimal;

        // Measurement checkpoint, make sure fsm with phase data is set up
        while (true) {
            struct belt_tensions tension_data(freq_top_belt, freq_bottom_belt);
            fsm_change(PhaseManualBeltTuning::measure_upper_belt, fsm::serialize_data<belt_tensions>(tension_data));

            vibrator.frequency = freq_top_belt;
            vibrator.excitation_acceleration = abs(calc_accel(vibrator.frequency) * 0.001f);

            // Contains loop that breaks with knob click
            resonate(vibrator, PhaseManualBeltTuning::measure_upper_belt);

            // Save top belt results
            freq_top_belt = vibrator.frequency;
            tension_data = belt_tensions(freq_top_belt, freq_bottom_belt);
            fsm_change(PhaseManualBeltTuning::measure_lower_belt, fsm::serialize_data<belt_tensions>(tension_data));

            vibrator.frequency = freq_bottom_belt;
            vibrator.excitation_acceleration = abs(calc_accel(vibrator.frequency) * 0.001f);

            // Contains loop that breaks with knob click
            resonate(vibrator, PhaseManualBeltTuning::measure_lower_belt);

            // Save bottom belt results
            freq_bottom_belt = vibrator.frequency;
            tension_data = belt_tensions(freq_top_belt, freq_bottom_belt);

            if (fabs(freq_top_belt - freq_bottom_belt) > freq_diff_limit) {
                fsm_change(PhaseManualBeltTuning::alignment_issue);
                switch (wait_for_response(PhaseManualBeltTuning::alignment_issue)) {
                case Response::Retry:
#if HAS_GANTRY_SQUARENESS_CHECK()
                    microstep_restorer.reset();
                    if (indx_gantry_squareness::run_wizard() == indx_gantry_squareness::WizardResult::aborted) {
                        return Result::abort;
                    }
#endif
                    return Result::retry;
                case Response::Abort:
                    return Result::abort;
                default:
                    bsod_unreachable();
                    return Result::abort;
                }
            }

            // eights of one turn (signed)
            int8_t rev_eights = 0; // Default value 0 (no turns needed)
            const float freq_higher = std::fmax(freq_top_belt, freq_bottom_belt);
            const float freq_lower = std::fmin(freq_bottom_belt, freq_top_belt);

            if (freq_higher > freq_result_max || freq_lower < freq_result_min) {
                // difference frequency
                const float dfh = higher_freq_belt_optimal - freq_higher;
                const float dfl = lower_freq_belt_optimal - freq_lower;
                // difference revelations
                const float drh = calc_revs_from_freq(dfh, dfl, belt_hz_per_rev, belt_hz_per_rev2);
                const float drl = calc_revs_from_freq(dfl, dfh, belt_hz_per_rev, belt_hz_per_rev2);
                //  average
                const float dr = (drh + drl) / 2;
                // eights of one turn (signed)
                rev_eights = static_cast<int8_t>(std::roundf(dr * 8));
            }

            fsm_change(PhaseManualBeltTuning::show_tension, fsm::serialize_data<belt_tensions>(tension_data));
            if (wait_for_continue(PhaseManualBeltTuning::show_tension)) {
                fsm_change(PhaseManualBeltTuning::adjust_tensioners, fsm::serialize_data<screw_revs>({ rev_eights }));
            } else {
                return Result::abort;
            }

            // Adjust tensioners
            switch (wait_for_response(PhaseManualBeltTuning::adjust_tensioners)) {
            case Response::Continue:
                break;
            case Response::Adjust:
                continue; // Restart measurements from top belt
            case Response::Abort:
                return Result::abort;
            default:
                bsod_unreachable();
                break;
            }
            break;
        }

#if HAS_GANTRY_SQUARENESS_CHECK()
        // Restore microsteps for the homing moves of the squareness check
        microstep_restorer.reset();

        // The tensioner adjustment can move the belts on the pulleys - verify
        // again, with the squareness wizard FSM popping up over this one
        switch (indx_gantry_squareness::run_wizard()) {
        case indx_gantry_squareness::WizardResult::passed:
            break;
        case indx_gantry_squareness::WizardResult::realigned:
            return Result::retry;
        case indx_gantry_squareness::WizardResult::aborted:
            return Result::abort;
        }
#endif

        fsm_change(PhaseManualBeltTuning::finished);
        return Result::pass;
    }

    Result run_from_gantry() {
#if HAS_AUTOMATIC_BELT_TUNING()
        if (accelerometer_available) {
            if (const auto result = run_automatic()) {
                return *result;
            }
            accelerometer_available = false;
        }
#endif
        return run_manual();
    }

    void run() {

        planner.finish_and_disable();
        FSM_Holder holder { PhaseManualBeltTuning::intro };
        if (!wait_for_continue(PhaseManualBeltTuning::intro)) {
            selftest_invocation::mark_aborted();
            return;
        }

        Result res = Result::abort;
        do {
            // Starting top belt tension
            res = run_from_gantry();
        } while (res == Result::retry);

        if (res == Result::abort) {
            selftest_invocation::mark_aborted();
            return;
        }

        // Single button finish screen
        wait_for_response(PhaseManualBeltTuning::finished);
        config_store().manual_belt_tuning_completed.set(true);
    }

    void adjust_vibrator(Vibrate &vibrator) {
        vibrator.frequency = std::clamp<float>(abs(vibrator.frequency), freq_min, freq_max);
        vibrator.excitation_acceleration = abs(calc_accel(vibrator.frequency) * 0.001f);
#if HAS_XBUDDY_EXTENSION()
        buddy::xbuddy_extension().set_strobe(vibrator.frequency + 4 /*taken from python script*/);
#endif
    }

    Response resonate(Vibrate &vibrator, PhaseManualBeltTuning phase) {
        Response response = Response::_none;
        auto knob_pos = get_knob_position();
        while (true) {
            {
                const auto new_knob_pos = get_knob_position();
                vibrator.frequency += (new_knob_pos - knob_pos) * 0.5f;
                knob_pos = new_knob_pos;
            }

            adjust_vibrator(vibrator);

            const struct belt_tensions tension_data(vibrator.frequency, vibrator.frequency); // duplicated - could be either of the belts
            fsm_change(phase, fsm::serialize_data<belt_tensions>(tension_data));

            vibrator.step();

            if ((response = get_response_from_phase(phase)) != Response::_none) {
                break;
            }

            idle(true);
        }
        return response;
    }

    bool wait_for_continue(PhaseManualBeltTuning phase) const {
        switch (wait_for_response(phase)) {
        case Response::Continue:
            return true;
        case Response::Abort:
            break;
        default:
            bsod_unreachable();
            break;
        }
        return false;
    }
};

} // namespace

void manual_belt_tuning::run_wizard() {
    ManualBeltTuningWizard wizard;
    wizard.run();
}
