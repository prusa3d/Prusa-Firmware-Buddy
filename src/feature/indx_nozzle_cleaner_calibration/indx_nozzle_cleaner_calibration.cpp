#include "indx_nozzle_cleaner_calibration.hpp"
#include <test_result.hpp>

#include <bsod/bsod.h>
#include <client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <common/timing.h>
#include <gcode/gcode.h>
#include <marlin_server.hpp>
#include <Marlin/src/Marlin.h>
#include <module/endstops.h>
#include <module/motion.h>
#include <module/planner.h>
#include <module/stepper.h>
#include <module/stepper/indirection.h>
#include <module/prusa/toolchanger.h>
#include <module/prusa/corexy_transform.hpp>
#include <logging/log.hpp>
#include <loadcell.hpp>
#include <config_store/store_instance.hpp>
#include <common/selftest_result.hpp>
#include <common/mapi/acceleration_limiter.hpp>
#include <common/mapi/calibration_preamble.hpp>
#include <common/mapi/parking.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>
#include <nozzle_cleaner.hpp>
#include <tool/hotend/hotend.hpp>
#include <utils/variant_utils.hpp>
#include <selftest/selftest_invocation.hpp>
#include <option/has_crash_detection.h>
#include <option/has_wastebin_fill_tracking.h>
#include <raii/scope_guard.hpp>

#if HAS_CRASH_DETECTION()
    #include <feature/prusa/crash_recovery.hpp>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <span>

LOG_COMPONENT_DEF(NozzleCleanerCalibration, logging::Severity::info);

using marlin_server::wait_for_response;

namespace indx_nozzle_cleaner_calibration {

/// Hotend is considered cool enough to touch below this temperature [°C]
static constexpr int16_t cooldown_safe_temperature_c = 50;

/// Max tolerance (+/-) in both axes
static constexpr float offset_tolerance_mm = 3.0f;

/// Loadcell touch approach feedrate for the X/Y measurements [mm/s] (matches G425 XY probing).
static constexpr feedRate_t probe_feedrate_mm_s = 3;

/// Settle time before taring the loadcell for a touch [ms] (matches G425 resonance damper wait).
static constexpr uint32_t probe_settle_ms = 500;

/// Number of loadcell touches per measurement (odd -> unambiguous median; >= 4 for the trimmed spread).
static constexpr uint8_t probe_sample_count = 5;
static_assert(probe_sample_count >= 4);

/// Max allowed trimmed spread between the touch samples [mm]; above this the touches are rejected as
/// inconsistent. INDX_TODO: tune.
static constexpr float probe_max_spread_mm = 0.4f;

/// XY probe trigger threshold for a touch [g], raised above the loadcell default (40 g) so the firm tray
/// edge is not triggered prematurely on approach vibration/noise. INDX_TODO: tune.
static constexpr float probe_xy_trigger_g = 80;

/// Nominal nozzle tip radius [mm]; (observed range 1.2-1.9)
static constexpr float nozzle_radius_estimate_mm = 3.2f / 2.f;
/// Max XY acceleration during the touches [mm/s^2] (matches G425 XY probing). At the default travel
/// acceleration, the jolt at the approach start can push the freshly tared loadcell over the trigger
/// threshold, producing a false contact right at the entry point.
static constexpr float probe_xy_acceleration_mm_s2 = 500;

/// Accepted range for the effective nozzle radius measured by the two-sided wall touch [mm]. The full
/// nozzle radius above the cone is ~2 mm; a larger measured radius means the loadcell triggered before
/// reaching the metal - most likely a plastic blob (nozzle not clean), or the wall was hit high up the
/// cone from a misadjusted Z screw. Below the min the touches were too close together (bogus).
static constexpr float nozzle_radius_min_mm = 1.0f;
static constexpr float nozzle_radius_max_mm = 2.1f;

/// Currently applied hotend offset (nozzle = carriage + offset); zero without hotend offset support.
static xy_float_t current_nozzle_offset() {
    xy_float_t offset = { 0.0f, 0.0f };
#if HAS_HOTEND_OFFSET
    offset = hotend_currently_applied_offset.xy();
#endif
    return offset;
}

/// Busy-wait while keeping the machine idle (needed so the loadcell keeps sampling before taring).
static void wait_ms(uint32_t duration_ms) {
    const uint32_t start = ticks_ms();
    while (ticks_ms() - start < duration_ms) {
        idle(true);
    }
}

/// Median plus a trimmed spread (range with the lowest and highest sample dropped, so one outlier touch
/// per side doesn't fail the consistency check). Sorts @p samples in place.
struct SampleStats {
    float median; ///< median sample [mm]
    float spread; ///< range of the samples with the extremes dropped [mm]
};

static SampleStats sample_stats(std::array<float, probe_sample_count> &samples) {
    std::sort(samples.begin(), samples.end());
    return {
        .median = samples[probe_sample_count / 2],
        .spread = samples[probe_sample_count - 2] - samples[1],
    };
}

/// Exit measurement point for the manual fallback: on Y, clear the cleaner in -X before homing.
static constexpr float exit_move_mm = -30.f;

/// One machine position [mm] a manual-fallback measurement may snap to. The extended-capacity wastebin
/// variant has its Y calibration indent 40 mm closer to the silicone blocks, so Y has two valid targets;
/// X has one.
struct CalibTarget {
    /// Expected position in machine coordinates [mm].
    float nominal_mm;
    /// When this target matches: set nozzle_cleaner_extended_capacity to this (identifies the installed
    /// bin variant). nullopt = leave the capacity unchanged (X carries no capacity information).
    std::optional<bool> set_extended_capacity;
};

/// Per-axis manual-fallback configuration.
struct AxisCalibConfig {
    PhaseNozzleCleanerCalibration phase_ask_position;
    PhaseNozzleCleanerCalibration phase_lock_position;
    PhaseNozzleCleanerCalibration phase_measuring;
    PhaseNozzleCleanerCalibration phase_evaluating;
    AxisEnum axis;
    /// Candidate positions the measurement may match (within tolerance). At most one can match: they are
    /// far enough apart (40 mm) that the +/- tolerance windows never overlap.
    std::span<const CalibTarget> targets;
    /// Park position before disabling motors so the user can jog the head onto the calibration feature.
    xy_pos_t park_pos;
};

static constexpr std::array x_targets {
    CalibTarget { .nominal_mm = X_NOZZLE_CLEANER_ORIGIN, .set_extended_capacity = std::nullopt },
};

static constexpr std::array y_targets {
    // Standard (longer) bin -> calibration indent at the origin, standard capacity.
    CalibTarget { .nominal_mm = Y_NOZZLE_CLEANER_CALIB_POINT_STANDARD, .set_extended_capacity = false },
    // Extended (shorter) bin -> indent 40 mm closer to the blocks, extended capacity.
    CalibTarget { .nominal_mm = Y_NOZZLE_CLEANER_CALIB_POINT_EXTENDED, .set_extended_capacity = true },
};

static constexpr AxisCalibConfig x_axis_config {
    .phase_ask_position = PhaseNozzleCleanerCalibration::ask_position_x,
    .phase_lock_position = PhaseNozzleCleanerCalibration::lock_position_x,
    .phase_measuring = PhaseNozzleCleanerCalibration::measuring_x,
    .phase_evaluating = PhaseNozzleCleanerCalibration::evaluating_x,
    .axis = AxisEnum::X_AXIS,
    .targets = x_targets,
    .park_pos = { .x = X_WASTEBIN_SAFE_POINT, .y = Y_BRUSH_AVOID_POINT },
};

static constexpr AxisCalibConfig y_axis_config {
    .phase_ask_position = PhaseNozzleCleanerCalibration::ask_position_y,
    .phase_lock_position = PhaseNozzleCleanerCalibration::lock_position_y,
    .phase_measuring = PhaseNozzleCleanerCalibration::measuring_y,
    .phase_evaluating = PhaseNozzleCleanerCalibration::evaluating_y,
    .axis = AxisEnum::Y_AXIS,
    .targets = y_targets,
    .park_pos = { .x = X_NOZZLE_CLEANER_ORIGIN - 10.f, .y = Y_NOZZLE_CLEANER_ORIGIN },
};

class NozzleCleanerCalibrationWizard {
public:
    void run() {
        const auto result = run_inner();

        // Store calibration result (abort leaves it unchanged)
        switch (result) {
        case Result::success:
            config_store().selftest_result_nozzle_cleaner_calibration.set(TestResult::passed);
            break;
        case Result::aborted:
            selftest_invocation::mark_aborted();
            break;
        }

        disable_XY();
        disable_Z();

        if (result == Result::success) {
            fsm_change(PhaseNozzleCleanerCalibration::calibration_success);
            wait_for_response(PhaseNozzleCleanerCalibration::calibration_success);
        }
    }

private:
    marlin_server::FSM_Holder holder { PhaseNozzleCleanerCalibration::intro };

    enum class Result {
        success,
        aborted,
    };

    void fsm_change(PhaseNozzleCleanerCalibration phase, fsm::PhaseData data = {}) {
        marlin_server::fsm_change(phase, data);
    }

    Result run_inner() {
        fsm_change(PhaseNozzleCleanerCalibration::intro);
        if (wait_for_response(PhaseNozzleCleanerCalibration::intro) == Response::Abort) {
            return Result::aborted;
        }

        const mapi::CalibrationPreamble preamble {
            .tool_policy = mapi::CalibrationPreamble::ToolPolicy::ensure_picked,
            .on_step = [&](mapi::CalibrationPreamble::Step step) {
                switch (step) {
                case mapi::CalibrationPreamble::Step::moving_away:
                    fsm_change(PhaseNozzleCleanerCalibration::moving_away);
                    break;
                case mapi::CalibrationPreamble::Step::picking_tool:
                    fsm_change(PhaseNozzleCleanerCalibration::picking_tool);
                    break;
                case mapi::CalibrationPreamble::Step::homing:
                    fsm_change(PhaseNozzleCleanerCalibration::homing);
                    break;
                case mapi::CalibrationPreamble::Step::parking_tool:
                    bsod_unreachable();
                }
            },
        };

        if (!preamble.run()) {
            return Result::aborted;
        }

        // Wait for the nozzle to cool down before the user handles the head
        if (const auto result = wait_for_nozzle_cooldown(); result != Result::success) {
            return result;
        }

        // One-time mechanical Z screw adjustment
        mapi::park({ .x = X_WASTEBIN_SAFE_POINT, .y = Y_BRUSH_AVOID_POINT });
        disable_XY();
        fsm_change(PhaseNozzleCleanerCalibration::move_to_z_point);
        if (wait_for_response(PhaseNozzleCleanerCalibration::move_to_z_point) == Response::Abort) {
            return Result::aborted;
        }

        // The Z screw step disabled XY, so re-home before the loadcell touches (they work in machine
        // coordinates and need a homed machine).
        fsm_change(PhaseNozzleCleanerCalibration::homing);
        GcodeSuite::G28_no_parser(true, true, false, { .z_raise = 0, .precise = false });

        // X (cleaner wall) first: the Y touch needs the measured wall middle and nozzle radius.
        {
            const auto result = calibrate_x_loadcell();
            if (result != Result::success) {
                return result;
            }
        }
        {
            const auto result = calibrate_y_loadcell();
            if (result != Result::success) {
                return result;
            }
        }

        return Result::success;
    }

    /// Wait until the currently selected hotend cools below a safe touch temperature.
    /// Pushes the current temperature to the GUI each idle tick.
    Result wait_for_nozzle_cooldown() {
        const auto tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
        if (!tool.has_value()) {
            bsod_unreachable();
        }
        auto &hotend = Hotend::for_tool(*tool);
        const auto is_cooled_down = [&hotend]() {
            auto temp = hotend.nozzle_temp();
            return temp.has_value() && temp.value() <= cooldown_safe_temperature_c;
        };

        const bool not_heating_above_safe = hotend.nozzle_target_temp() <= cooldown_safe_temperature_c;
        if (is_cooled_down() && not_heating_above_safe) {
            return Result::success;
        }

        fsm_change(PhaseNozzleCleanerCalibration::wait_for_nozzle_cooldown);

        // Interrupt the blocking wait the instant Abort is pressed. The handler resumes queuing on
        // scope exit. Cooldown does no moves, so there are no skipped steps to recover.
        GCodeExceptionHandler abort_handler { GCEHandlerExtent::extruder_only, [] {} };

        // Push current temp to the GUI and handle abort while waiting to cool down
        Subscriber subscriber(marlin_server::idle_publisher, [&hotend, &abort_handler] {
            if (marlin_server::get_response_from_phase(PhaseNozzleCleanerCalibration::wait_for_nozzle_cooldown) == Response::Abort) {
                gcode_exceptions().throw_at(&abort_handler);
                return;
            }
            // Read the live temperature each tick so the GUI reflects the actual cooldown progress
            auto curr_temp = hotend.nozzle_temp();
            if (!curr_temp.has_value()) {
                return;
            }
            const uint16_t t = static_cast<uint16_t>(curr_temp.value());
            const fsm::PhaseData data = {
                static_cast<uint8_t>((t >> 8) & 0xff),
                static_cast<uint8_t>(t & 0xff),
                0,
                0,
            };
            marlin_server::fsm_change(PhaseNozzleCleanerCalibration::wait_for_nozzle_cooldown, data);
        });

        hotend.set_nozzle_target_temp(0);
        thermalManager.set_print_fan_speed(255);
        ScopeGuard fan_guard([]() {
            thermalManager.set_print_fan_speed(0);
        });
        while (!is_cooled_down() && !gcode_exceptions().is_unwinding()) {
            idle(true);
        }

        return gcode_exceptions().is_unwinding() ? Result::aborted : Result::success;
    }

    /// Wall middle X and effective nozzle radius from the two-sided X wall measurement of this cycle.
    /// nullopt after the manual X fallback (no loadcell touches) - the single-sided Y touch needs both,
    /// so Y then goes manual as well.
    std::optional<float> wall_middle_x;
    std::optional<float> nozzle_radius;

    /// Single-axis machine move at the slow toolchanger feedrate.
    void machine_move_axis(AxisEnum axis, float pos_mm) {
        auto target = current_machine_position();
        target[axis] = pos_mm;
        line_to_machine_pos(target, PrusaToolChanger::SLOW_MOVE_MM_S);
        planner.synchronize();
    }

    /// Move the head to the Y touch entry point: measured wall middle in X, entry Y, current Z (the fixed
    /// cleaner spans the Z range, so the nozzle Z relative to the block does not matter). Called with the
    /// head either on the purge-entry lane (only X changes) or retreating from a touch (only Y changes),
    /// so the move never cuts across the tray.
    void move_to_purge_touch_entry() {
        auto target = current_machine_position();
        target.x = *wall_middle_x - current_nozzle_offset().x;
        target.y = Y_NOZZLE_CLEANER_PURGE_ENTRY;
        line_to_machine_pos(target, PrusaToolChanger::SLOW_MOVE_MM_S);
        planner.synchronize();
    }

    /// Retreat clear of the cleaner after the Y touches: back out to the entry point (clear of the tray
    /// back edge in +Y), then step aside in -X. Any following move down in Y (manual Y park, wizard exit)
    /// then clears the cleaner instead of cutting a diagonal across it.
    void move_out_after_purge_touch() {
        move_to_purge_touch_entry();
        auto target = current_machine_position();
        target.x = X_NOZZLE_CLEANER_ORIGIN - 10.f;
        line_to_machine_pos(target, PrusaToolChanger::SLOW_MOVE_MM_S);
        planner.synchronize();
    }

    /// Single loadcell touch along @p axis toward @p probe_target_mm (a machine coordinate on that
    /// axis); see G425::probe_xy for the same pattern. The caller must be positioned at the entry point
    /// and must retreat afterwards.
    /// @return the contact position on @p axis in machine coordinates, folded with the picked tool's
    ///         hotend offset so the value is tool-independent, or nullopt if the target was never reached.
    std::optional<float> touch_axis(AxisEnum axis, float probe_target_mm) {
        loadcell.set_xy_endstop(true);
        Loadcell::XyProbeThresholdOverride firm_touch(loadcell, probe_xy_trigger_g);
        wait_ms(probe_settle_ms);
        {
            // Arm for tare + approach only; the retreat afterwards must run unarmed.
            Loadcell::ProbeSafetyArmer safety_armer(loadcell);
            loadcell.WaitBarrier(); // Sync samples before tare
            loadcell.Tare(Loadcell::TareMode::Continuous);
            if (loadcell.GetXYEndstop()) {
                bsod("XY probe triggered");
            }

            endstops.enable_xy_probe(true);
#if HAS_CRASH_DETECTION()
            crash_s.deactivate();
#endif
            auto target = current_machine_position();
            target[axis] = probe_target_mm;
            line_to_machine_pos(target, probe_feedrate_mm_s);
            planner.synchronize();
        }

        const bool reached = endstops.trigger_state();
#if HAS_CRASH_DETECTION()
        crash_s.activate();
#endif
        loadcell.set_xy_endstop(false);
        endstops.enable_xy_probe(false);

        if (!reached) {
            // A loadcell safety-stop quick-stops mid-move without setting the endstop, leaving the planner
            // position at the un-reached target; resync it so the following retreat isn't run from a stale origin.
            planner.reset_position();
            return std::nullopt;
        }

        endstops.hit_on_purpose();
        planner.reset_position();
        const ab_steps_t hit_steps = { stepper.position(A_AXIS), stepper.position(B_AXIS) };
        MachinePosXYZE hit_mm;
        corexy_ab_to_xyze(hit_steps, hit_mm);

        // Fold the picked tool's hotend offset in (nozzle = carriage + offset), making the value
        // tool-independent. Every carriage move target derived from it must unfold the offset again.
        return hit_mm[axis] + current_nozzle_offset()[axis];
    }

    /// Result of several loadcell touches along one axis.
    struct AxisMeasurement {
        bool reached; ///< false if any touch failed to reach the block
        float median; ///< median contact position [mm] (valid only if reached)
        float spread; ///< trimmed spread of the samples [mm] (consistency check; valid only if reached)
    };

    /// Take @c probe_sample_count loadcell touches along @p axis toward @p probe_target_mm, robust to a
    /// single bad touch via the median + trimmed spread. @p retreat runs after each touch to back the head
    /// clear of the block before the next one; @p label names the axis in the logs.
    template <typename Retreat>
    AxisMeasurement measure_axis(AxisEnum axis, const char *label, float probe_target_mm, Retreat &&retreat) {
        std::array<float, probe_sample_count> samples;
        Loadcell::HighPrecisionEnabler high_precision(loadcell);
        mapi::AccelerationLimiter accel_limiter(probe_xy_acceleration_mm_s2);
        for (uint8_t i = 0; i < probe_sample_count; ++i) {
            const auto contact = touch_axis(axis, probe_target_mm);
            retreat();
            if (!contact.has_value()) {
                log_error(NozzleCleanerCalibration,
                    "Nozzle cleaner %s touch %hhu did not reach the target (probe limit %.2f mm)",
                    label, i, static_cast<double>(probe_target_mm));
                return { .reached = false, .median = 0, .spread = 0 };
            }
            log_info(NozzleCleanerCalibration, "Nozzle cleaner %s touch %hhu: %.2f mm",
                label, i, static_cast<double>(*contact));
            samples[i] = *contact;
        }
        const auto stats = sample_stats(samples);
        return { .reached = true, .median = stats.median, .spread = stats.spread };
    }

    /// Move the head to the outer wall touch entry point: X first, then Y, so the approach runs down the
    /// entry lane (clear of the wall over its whole Y range) instead of cutting a diagonal across the
    /// cleaner - a retry may start from the purge-entry lane after a failed inner touch. Current Z (the
    /// fixed cleaner spans the Z range, so the nozzle Z relative to the wall does not matter). X is not
    /// calibrated yet at this point, so no offset is applied.
    void move_to_wall_touch_entry() {
        machine_move_axis(AxisEnum::X_AXIS, X_NOZZLE_CLEANER_WALL_ENTRY);
        machine_move_axis(AxisEnum::Y_AXIS, X_NOZZLE_CLEANER_WALL_TOUCH_Y);
    }

    /// Shared per-axis calibration loop: run @p run_automatic once, and on failure show the results
    /// screen offering manual calibration (Yes), an automatic retry (Retry), or aborting (Abort). Both
    /// @p run_automatic and the manual fallback write @p offset / @p nominal for the results screen and
    /// return nullopt on failure, or a terminal Result otherwise. @p use_manual starts with the manual
    /// flow directly, skipping the automatic attempt.
    template <typename Automatic>
    Result run_axis_calibration(const AxisCalibConfig &config, Automatic &&run_automatic, bool use_manual = false) {
        for (;;) {
            // nullopt until an attempt records a value; stays nullopt when the probe never touches,
            // which the results screen renders as "N/A" instead of a misleading 0.00 mm.
            std::optional<float> offset;
            float nominal = 0.f;
            const std::optional<Result> result = use_manual
                ? run_manual_attempt(config, offset, nominal)
                : run_automatic(offset, nominal);
            if (result.has_value()) {
                return *result;
            }

            // Measurement failed -> results screen: Yes = calibrate by hand, Retry = automatic again, Abort = cancel.
            fsm_change(config.phase_evaluating, fsm::serialize_data(EvaluatingData::from(offset, nominal)));
            switch (wait_for_response(config.phase_evaluating)) {
            case Response::Retry:
                use_manual = false;
                break;
            case Response::Yes:
                use_manual = true;
                break;
            default:
                return Result::aborted;
            }
        }
    }

    /// Automatic X calibration: touch the cleaner wall with the loadcell from both sides.
    ///
    /// Runs first (the Y touch needs its results) and after a G28 (machine homed). The outer face is
    /// touched in +X from the entry lane; the head then travels around the wall's +Y end (up to the
    /// purge-entry lane, over to the V-groove lane and down past the wastebin) and touches the inner
    /// face in -X. The mean of the two contact medians is the wall middle - the nozzle radius cancels
    /// out, so the result does not depend on how high up the nozzle cone the wall is hit (i.e. on the
    /// mechanical Z adjustment). The contact distance also yields this cycle's effective nozzle radius,
    /// which the Y touch needs to compensate its single-sided measurement.
    ///
    /// On failure the results screen offers a manual calibration (Yes), an automatic retry (Retry), or
    /// aborting; failures always leave the head clear of the cleaner (on the outer entry lane or the
    /// purge-entry lane).
    /// @return success if calibrated (automatically or by hand), aborted if the user cancels
    Result calibrate_x_loadcell() {
        return run_axis_calibration(x_axis_config, [this](std::optional<float> &offset, float &nominal) -> std::optional<Result> {
            // Loop so a too-large radius (dirty nozzle) can be re-measured after the user cleans it,
            // without dropping to the manual results screen.
            for (;;) {
                offset.reset(); // each attempt starts unmeasured; the "no contact" paths below rely on this
                nominal = X_NOZZLE_CLEANER_WALL_MIDDLE_NOMINAL;
                fsm_change(PhaseNozzleCleanerCalibration::measuring_x);

                // Outer face: touch in +X from the entry, retreating to the entry between touches.
                move_to_wall_touch_entry();
                const auto outer = measure_axis(AxisEnum::X_AXIS, "X outer", X_NOZZLE_CLEANER_WALL_PROBE_MAX,
                    [this] { move_to_wall_touch_entry(); });

                if (!outer.reached) {
                    // No contact -> leave offset unset so the results screen shows "N/A", not a bogus 0.00 mm.
                    log_error(NozzleCleanerCalibration, "Nozzle cleaner X outer touch did not reach the wall");
                    return std::nullopt;
                }
                if (outer.spread > probe_max_spread_mm) {
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner X outer touches inconsistent: spread %.2f mm (max %.2f mm)",
                        static_cast<double>(outer.spread), static_cast<double>(probe_max_spread_mm));
                    return std::nullopt;
                }

                // Cleaner offset estimated from the outer contact alone (assuming the nominal tip radius);
                // aligns the V-groove lane and the inner probe with the real part position before driving
                // in. The radius uncertainty is far below the lane clearances.
                const float estimated_offset = outer.median - (X_NOZZLE_CLEANER_WALL_OUTER_FACE_NOMINAL - nozzle_radius_estimate_mm);
                if (std::abs(estimated_offset) > offset_tolerance_mm) {
                    offset = estimated_offset;
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner X offset estimate %.2f out of bounds (max %.1f mm)",
                        static_cast<double>(estimated_offset), static_cast<double>(offset_tolerance_mm));
                    return std::nullopt;
                }

                // Around the wall's +Y end to its inner side: up the entry lane to the purge-entry lane,
                // over to the V-groove lane and down past the wastebin to the touch Y. The estimate is a
                // nozzle-frame value, so the carriage targets derived from it unfold the tool offset.
                const float lane_x = X_NOZZLE_CLEANER_ORIGIN + estimated_offset - current_nozzle_offset().x;
                machine_move_axis(AxisEnum::Y_AXIS, Y_NOZZLE_CLEANER_PURGE_ENTRY);
                machine_move_axis(AxisEnum::X_AXIS, lane_x);
                machine_move_axis(AxisEnum::Y_AXIS, X_NOZZLE_CLEANER_WALL_TOUCH_Y);

                // Inner face: touch in -X from the V-groove lane; the probe limit keeps the nozzle center
                // from ever crossing the (estimated) wall middle.
                const auto inner = measure_axis(AxisEnum::X_AXIS, "X inner",
                    X_NOZZLE_CLEANER_WALL_MIDDLE_NOMINAL + estimated_offset - current_nozzle_offset().x,
                    [this, lane_x] { machine_move_axis(AxisEnum::X_AXIS, lane_x); });

                // Back out to the purge-entry lane right away so every following state - results screen,
                // retry, manual park, the Y touch - starts clear of the wastebin interior.
                machine_move_axis(AxisEnum::Y_AXIS, Y_NOZZLE_CLEANER_PURGE_ENTRY);

                if (!inner.reached) {
                    log_error(NozzleCleanerCalibration, "Nozzle cleaner X inner touch did not reach the wall");
                    return std::nullopt;
                }

                const float middle = (outer.median + inner.median) / 2.f;
                const float radius = (inner.median - outer.median - X_NOZZLE_CLEANER_WALL_THICKNESS) / 2.f;
                offset = middle - X_NOZZLE_CLEANER_WALL_MIDDLE_NOMINAL;

                if (inner.spread > probe_max_spread_mm) {
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner X inner touches inconsistent: spread %.2f mm (max %.2f mm)",
                        static_cast<double>(inner.spread), static_cast<double>(probe_max_spread_mm));
                    return std::nullopt;
                }
                if (radius > nozzle_radius_max_mm) {
                    // Radius too large -> the loadcell most likely triggered on a plastic blob instead of
                    // the metal cone: the nozzle is not clean (a wall hit high up the cone from a misadjusted
                    // Z screw reads the same). Park somewhere reachable, ask the user to clean it, re-measure.
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner effective nozzle radius %.2f too large (max %.1f mm); nozzle probably not clean",
                        static_cast<double>(radius), static_cast<double>(nozzle_radius_max_mm));
                    // Center X, in front of the docks -> nozzle reachable through the front door for cleaning.
                    mapi::park({ .x = X_BED_SIZE / 2.f, .y = Y_DOCK_PARKING_MIN_SAFE_POS });
                    fsm_change(PhaseNozzleCleanerCalibration::clean_nozzle);
                    if (wait_for_response(PhaseNozzleCleanerCalibration::clean_nozzle) == Response::Abort) {
                        return Result::aborted;
                    }
                    continue; // Retry: re-measure (still automatic)
                }
                if (radius < nozzle_radius_min_mm) {
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner effective nozzle radius %.2f too small (min %.1f mm); check the Z screw adjustment",
                        static_cast<double>(radius), static_cast<double>(nozzle_radius_min_mm));
                    return std::nullopt;
                }
                if (std::abs(*offset) > offset_tolerance_mm) {
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner X offset %.2f out of bounds (max %.1f mm)",
                        static_cast<double>(*offset), static_cast<double>(offset_tolerance_mm));
                    return std::nullopt;
                }

                config_store().nozzle_cleaner_x_origin_offset.set(*offset);
                wall_middle_x = middle;
                nozzle_radius = radius;
                log_info(NozzleCleanerCalibration,
                    "Nozzle cleaner X calibrated: offset=%.2f middle=%.2f radius=%.2f (spread outer %.2f inner %.2f)",
                    static_cast<double>(*offset), static_cast<double>(middle), static_cast<double>(radius),
                    static_cast<double>(outer.spread), static_cast<double>(inner.spread));
                return Result::success;
            }
        });
    }

    /// Automatic Y calibration: touch the back edge of the plastic cleaner tray with the loadcell.
    ///
    /// The head drives to the measured wall middle at the entry Y and touches the tray's back edge in -Y
    /// several times, retreating between touches; the median contact minus this cycle's effective nozzle
    /// radius is the physical edge position, and minus the nominal gives the stored, origin-relative Y
    /// offset for G750.
    ///
    /// Runs after X is calibrated and after a G28 (machine homed). The single-sided touch needs the wall
    /// middle and nozzle radius from the two-sided X measurement, so after a manual X calibration the
    /// manual Y flow starts directly. On failure the results screen offers a manual calibration (Yes),
    /// an automatic retry (Retry), or aborting.
    /// @return success if calibrated (automatically or by hand), aborted if the user cancels
    Result calibrate_y_loadcell() {
        const bool have_wall_measurement = wall_middle_x.has_value() && nozzle_radius.has_value();
        return run_axis_calibration(
            y_axis_config, [this](std::optional<float> &offset, float &nominal) -> std::optional<Result> {
                nominal = Y_NOZZLE_CLEANER_PURGE_BACK_NOMINAL;
                if (!wall_middle_x.has_value() || !nozzle_radius.has_value()) {
                    log_error(NozzleCleanerCalibration, "Nozzle cleaner Y touch needs the X wall measurement, calibrate manually");
                    return std::nullopt;
                }
                fsm_change(PhaseNozzleCleanerCalibration::measuring_y);

                // Tray back edge: touch in -Y from the entry (failures leave the head at the entry, clear).
                move_to_purge_touch_entry();
                const auto back = measure_axis(AxisEnum::Y_AXIS, "Y", Y_NOZZLE_CLEANER_PURGE_PROBE_MIN,
                    [this] { move_to_purge_touch_entry(); });

                if (!back.reached) {
                    // No contact -> leave offset unset so the results screen shows "N/A", not a bogus 0.00 mm.
                    log_error(NozzleCleanerCalibration, "Nozzle cleaner Y touch did not reach the tray back edge");
                    return std::nullopt;
                }
                // The nozzle center stops one effective radius short of the edge face.
                offset = back.median - *nozzle_radius - Y_NOZZLE_CLEANER_PURGE_BACK_NOMINAL;
                if (back.spread > probe_max_spread_mm) {
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner Y back touches inconsistent: spread %.2f mm (max %.2f mm)",
                        static_cast<double>(back.spread), static_cast<double>(probe_max_spread_mm));
                    return std::nullopt;
                }
                if (std::abs(*offset) > offset_tolerance_mm) {
                    log_error(NozzleCleanerCalibration,
                        "Nozzle cleaner Y offset %.2f out of bounds (max %.1f mm)",
                        static_cast<double>(*offset), static_cast<double>(offset_tolerance_mm));
                    return std::nullopt;
                }

                config_store().nozzle_cleaner_y_origin_offset.set(*offset);
                log_info(NozzleCleanerCalibration,
                    "Nozzle cleaner Y calibrated: offset=%.2f (median %.2f radius %.2f spread %.2f)",
                    static_cast<double>(*offset), static_cast<double>(back.median),
                    static_cast<double>(*nozzle_radius), static_cast<double>(back.spread));

                // Exit clear of the cleaner before the head is driven down in Y (manual Y park / wizard exit).
                move_out_after_purge_touch();
                return Result::success;
            },
            !have_wall_measurement);
    }

    /// One manual measurement attempt for @p config (offered as the "Yes" branch of the results screen).
    /// Stores the offset on success. On an out-of-bounds result it writes @p offset / @p nominal for the
    /// results screen and returns nullopt so the caller re-shows it.
    /// @return success (calibrated by hand), aborted (user abort), or nullopt (out of bounds)
    std::optional<Result> run_manual_attempt(const AxisCalibConfig &config, std::optional<float> &offset, float &nominal) {
        const auto measured = manual_measure_axis(config);
        if (!measured.has_value()) {
            return Result::aborted;
        }

        // Snap the measurement to one of this axis' calibration targets (each within +/- tolerance).
        // The targets are far enough apart that at most one matches; keep the nearest for the failure UI.
        const CalibTarget *matched = nullptr;
        const CalibTarget *nearest = &config.targets.front();
        for (const CalibTarget &target : config.targets) {
            if (std::abs(*measured - target.nominal_mm) < std::abs(*measured - nearest->nominal_mm)) {
                nearest = &target;
            }
            if (std::abs(*measured - target.nominal_mm) <= offset_tolerance_mm) {
                matched = &target;
            }
        }

        // No target in range -> out of bounds. Report against the nearest and let the caller re-offer.
        if (matched == nullptr) {
            const float manual_offset = *measured - nearest->nominal_mm;
            offset = manual_offset;
            nominal = nearest->nominal_mm;
            log_error(NozzleCleanerCalibration,
                "Nozzle cleaner %c manual offset %.2f out of bounds (max %.1f mm)",
                (config.axis == AxisEnum::X_AXIS) ? 'X' : 'Y',
                static_cast<double>(manual_offset), static_cast<double>(offset_tolerance_mm));
            return std::nullopt;
        }

        // Manual measurement always yields a value (homing displacement, no probe), so the offset here is
        // never "not measured"; the results screen is not shown on success, so the out-param stays untouched.
        const float matched_offset = *measured - matched->nominal_mm;
        if (config.axis == AxisEnum::X_AXIS) {
            config_store().nozzle_cleaner_x_origin_offset.set(matched_offset);
        } else {
            config_store().nozzle_cleaner_y_origin_offset.set(matched_offset);
        }

#if HAS_WASTEBIN_FILL_TRACKING()
        // The matched Y target also identifies the installed wastebin variant -> set its capacity.
        // (Manual override stays available via the wastebin menu.)
        if (matched->set_extended_capacity.has_value()) {
            config_store().nozzle_cleaner_extended_capacity.set(*matched->set_extended_capacity);
        }
#endif

        log_info(NozzleCleanerCalibration, "Nozzle cleaner %c calibrated manually: offset=%.2f",
            (config.axis == AxisEnum::X_AXIS) ? 'X' : 'Y', static_cast<double>(matched_offset));
        return Result::success;
    }

    /// Manual measurement of one axis: the user jogs the nozzle onto the calibration feature (V-groove for
    /// X, indent for Y) and confirms, then a homing move measures the carriage displacement (no loadcell,
    /// so this works even when the touch fails). @return the contact position folded with the picked
    /// tool's hotend offset (tool-independent), or nullopt on user abort.
    std::optional<float> manual_measure_axis(const AxisCalibConfig &config) {
        // Pre-park near the calibration feature so the manual jog is short (the Y indent depends on the
        // currently configured bin variant).
        xy_pos_t park_pos = config.park_pos;
#if HAS_WASTEBIN_FILL_TRACKING()
        if (config.axis == AxisEnum::Y_AXIS) {
            park_pos.y = config_store().nozzle_cleaner_extended_capacity.get()
                ? Y_NOZZLE_CLEANER_CALIB_POINT_EXTENDED
                : Y_NOZZLE_CLEANER_CALIB_POINT_STANDARD;
        }
#endif
        mapi::park({ .x = park_pos.x, .y = park_pos.y });

        // Position + confirm loop — the user can go Back to reposition.
        for (;;) {
            // Disable motors so the user can jog the head by hand.
            planner.synchronize();
            disable_XY();

            fsm_change(config.phase_ask_position);
            if (wait_for_response(config.phase_ask_position) == Response::Abort) {
                return std::nullopt;
            }

            // Re-enable motors to lock the position, then confirm.
            enable_XY();
            fsm_change(config.phase_lock_position);
            const auto response = wait_for_response(config.phase_lock_position);
            if (response == Response::Abort) {
                return std::nullopt;
            }
            if (response == Response::Continue) {
                break; // Position confirmed, proceed to measurement.
            }
            // Response::Back -> loop back to repositioning.
        }

        fsm_change(config.phase_measuring);

        // Reset the current position to 0 on both axes as the reference point.
        planner.synchronize();
        current_position.x = 0;
        current_position.y = 0;
        sync_plan_position();

        const ab_steps_t position_before = { { {
            .x = stepper.position_from_startup(AxisEnum::A_AXIS),
            .y = stepper.position_from_startup(AxisEnum::B_AXIS),
        } } };

        // On the Y feature, move out in -X to clear the cleaner before homing. The X feature is in the V
        // groove; G28 homes Y first (away from the cleaner) then X, so no exit move is needed there.
        if (config.axis == AxisEnum::Y_AXIS) {
            auto target = current_machine_position();
            target.x += exit_move_mm;
            line_to_machine_pos(target, PrusaToolChanger::SLOW_MOVE_MM_S);
            planner.synchronize();
        }

        // Home XY (z_raise=0: Z is at the bottom, no need for Z clearance).
        GcodeSuite::G28_no_parser(true, true, false, { .z_raise = 0, .precise = false });

        const ab_steps_t position_after = { { {
            .x = stepper.position_from_startup(AxisEnum::A_AXIS),
            .y = stepper.position_from_startup(AxisEnum::B_AXIS),
        } } };

        // Convert the AB stepper difference to XY mm (CoreXY transform).
        const MachinePosXY diff = corexy_ab_to_xy(position_before - position_after);

        // Fold the picked tool's hotend offset in so the stored position is tool-independent.
        return (config.axis == AxisEnum::X_AXIS)
            ? (diff.x + current_position.x + current_nozzle_offset().x)
            : (diff.y + current_position.y + current_nozzle_offset().y);
    }
};

void run() {
    NozzleCleanerCalibrationWizard wizard;
    wizard.run();
}

} // namespace indx_nozzle_cleaner_calibration
