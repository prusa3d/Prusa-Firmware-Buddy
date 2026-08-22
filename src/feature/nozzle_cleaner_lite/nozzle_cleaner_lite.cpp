#include "include/nozzle_cleaner_lite.hpp"
#include "cleaner_zigzag.hpp"
#include <printers.h>
#include "Marlin/src/gcode/gcode.h"
#include <Marlin/src/Marlin.h>
#include <Marlin/src/module/motion.h>
#include <Marlin/src/module/planner.h>
#include <Marlin/src/module/probe.h>
#include <Marlin/src/module/temperature.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/tool_change.h>
    #include <module/prusa/toolchanger.h>
    #include <gcode/gcode_info.hpp>
#endif
#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif
#include <tool/hotend/hotend.hpp>
#include <Marlin/src/feature/pressure_advance/pressure_advance_config.hpp>
#include "loadcell.hpp"
#include <feature/print_status_message/print_status_message_guard.hpp>
#include <logging/log.hpp>
#include <raii/scope_guard.hpp>
#include <config_store/store_definition.hpp>
#include <bsod/bsod.h>
#include <timing.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <limits>

LOG_COMPONENT_DEF(NozzleCleanerLite, logging::Severity::info);

namespace nozzle_cleaner_lite {

namespace {
    enum class CleanerAxis : uint8_t {
        x,
        y,
    };

#if PRINTER_IS_PRUSA_XL()
    // Touchpoint: fixed reference point on the cleaner
    constexpr xy_pos_t touchpoint_xy = { { { -6.45f, 70.0f } } };

    constexpr CleanerAxis cleaner_axis = CleanerAxis::y;
    constexpr float cleaner_distance = 10.0f;
    constexpr float cleaner_length = 30.0f;

    constexpr float across_offset = -1.0f;
#elif PRINTER_IS_PRUSA_COREONE()
    constexpr xy_pos_t touchpoint_xy = { { { 206.5f, -15.0f } } };

    constexpr CleanerAxis cleaner_axis = CleanerAxis::x;
    constexpr float cleaner_distance = -10.0f;
    constexpr float cleaner_length = -30.0f;

    constexpr float across_offset = -1.5f;
#elif PRINTER_IS_PRUSA_COREONEL()
    constexpr xy_pos_t touchpoint_xy = { { { 299.5f, -7.5f } } };

    constexpr CleanerAxis cleaner_axis = CleanerAxis::x;
    constexpr float cleaner_distance = -10.0f;
    constexpr float cleaner_length = -30.0f;

    constexpr float across_offset = 0.0f;
#else
    #error "nozzle_cleaner_lite sequence not defined for this printer variant"
#endif

    // The pad's length runs along cleaner_axis and its width across the other
    // axis. Everything below is expressed in those two, so nothing has to ask
    // again which machine axis is which.
    constexpr bool pad_length_runs_along_x = (cleaner_axis == CleanerAxis::x);

    constexpr float touchpoint_along = pad_length_runs_along_x ? touchpoint_xy.x : touchpoint_xy.y;
    constexpr float touchpoint_across = pad_length_runs_along_x ? touchpoint_xy.y : touchpoint_xy.x;

    constexpr float along_travel_min = pad_length_runs_along_x ? X_MIN_POS : Y_MIN_POS;
    constexpr float along_travel_max = pad_length_runs_along_x ? X_MAX_POS : Y_MAX_POS;
    constexpr float across_travel_min = pad_length_runs_along_x ? Y_MIN_POS : X_MIN_POS;
    constexpr float across_travel_max = pad_length_runs_along_x ? Y_MAX_POS : X_MAX_POS;

    // 3mm sits comfortably inside the 7mm pad on every printer; only where that
    // band sits differs, because the travel left around the centre line does.
    constexpr float used_pad_width = 3.0f;
    constexpr float pad_along_near = touchpoint_along + cleaner_distance;
    constexpr float pad_across_min = touchpoint_across + across_offset;
    constexpr Pad cleaner_pad {
        .along_near = pad_along_near,
        .along_far = pad_along_near + cleaner_length,
        .across_min = pad_across_min,
        .across_max = pad_across_min + used_pad_width,
    };

    // Fractional on purpose: consecutive strokes take different diagonals instead
    // of every stroke retracing one. At 1.5 a stroke holds two or three diagonal
    // segments and the pattern comes back around every fourth stroke.
    constexpr float crossings_per_stroke = 1.5f;
    // A non-positive value would make the zigzag drift walk away from the stroke end without ever terminating.
    static_assert(crossings_per_stroke > 0.0f);

    constexpr float travel_limit_margin = 0.4f;
    static_assert(std::min(cleaner_pad.along_near, cleaner_pad.along_far) - travel_limit_margin >= along_travel_min);
    static_assert(std::max(cleaner_pad.along_near, cleaner_pad.along_far) + travel_limit_margin <= along_travel_max);
    static_assert(cleaner_pad.across_min - travel_limit_margin >= across_travel_min);
    static_assert(cleaner_pad.across_max + travel_limit_margin <= across_travel_max);

    // Z targets relative to the probed touchpoint surface
    constexpr float safe_above_surface_mm = 2.0f;
    constexpr float wipe_above_surface_mm = 0.2f;
    constexpr float travel_clearance_mm = 10.0f;
    constexpr float final_clearance_mm = 20.0f;

    constexpr feedRate_t approach_feedrate = MMM_TO_MMS(1200);
    constexpr feedRate_t leave_feedrate = MMM_TO_MMS(1200);
    constexpr feedRate_t dive_feedrate = MMM_TO_MMS(6000);
    constexpr feedRate_t rub_feedrate_fast = MMM_TO_MMS(8000);
    constexpr feedRate_t rub_feedrate_slow = MMM_TO_MMS(1500);

    constexpr uint8_t rub_cycles_fast = 5;
    constexpr uint8_t rub_cycles_slow = 2;
    constexpr float rub_acceleration = 5000.0f;

    // Move in raw machine coordinates (line_to_machine_pos), bypassing MBL.
    void move_to_machine_pos_xy(float x, float y, feedRate_t fr_mm_s) {
        auto target = current_machine_position();
        target.x = x;
        target.y = y;
        line_to_machine_pos(target, fr_mm_s);
        planner.synchronize();
    }

    void move_to_machine_pos_z(float z, feedRate_t fr_mm_s) {
        auto target = current_machine_position();
        target.z = z;
        line_to_machine_pos(target, fr_mm_s);
        planner.synchronize();
    }

    /// machine Z where the nozzle touched the touchpoint, nan if it never did.
    float find_touchpoint_z() {
        pressure_advance::PressureAdvanceDisabler pa_disabler;
        Loadcell::HighPrecisionEnabler loadcell_high_precision_enabler(loadcell);

        if (!do_homing_move(Z_AXIS, Z_PROBE_LOW_POINT - current_machine_position().z)) {
            log_error(NozzleCleanerLite, "touchpoint not found above Z %.1fmm", static_cast<double>(Z_PROBE_LOW_POINT));
            return std::numeric_limits<float>::quiet_NaN();
        }

        return current_machine_position().z;
    }

    // Queue a move given in pad coordinates.
    void line_to_pad(float along, float across, feedRate_t fr_mm_s) {
        auto target = current_machine_position();
        if constexpr (pad_length_runs_along_x) {
            target.x = along;
            target.y = across;
        } else {
            target.x = across;
            target.y = along;
        }
        line_to_machine_pos(target, fr_mm_s);
    }

    void move_to_pad(float along, float across, feedRate_t fr_mm_s) {
        line_to_pad(along, across, fr_mm_s);
        planner.synchronize();
    }

    // The waypoints are only queued, so the nozzle blends through the zigzag's
    // turns instead of stopping on the pad at each one.
    void rub_stroke(ZigZag &zigzag, float along_to, feedRate_t fr_mm_s) {
        zigzag.begin_stroke(along_to);
        while (const auto waypoint = zigzag.next()) {
            line_to_pad(waypoint->along, waypoint->across, fr_mm_s);
        }
        planner.synchronize();
    }

} // namespace

bool is_available() {
    return config_store().nozzle_cleaner_lite_installed.get();
}

bool clean(CleanArgs args) {
    release_assert(is_available());

    if (args.cleaning_temp < EXTRUDE_MINTEMP) {
        log_error(NozzleCleanerLite, "no cleaning temperature set");
        return false;
    }

    const int16_t rest_temp = args.probe_temp.value_or(args.cleaning_temp - cooldown_temp_diff);
    release_assert(!args.cooldown || rest_temp <= args.cleaning_temp);

    PrintStatusMessageGuard status_message;
    status_message.update<PrintStatusMessage::nozzle_cleaner_lite>({});

    const std::optional<PhysicalToolIndex> tool = PhysicalToolIndex::currently_selected_opt();
    if (!tool) {
        log_error(NozzleCleanerLite, "no tool selected");
        return false;
    }

    // Start heating used tool and restore target temp on exit
    const int16_t saved_nozzle_target = Hotend::for_tool(*tool).nozzle_target_temp();
    Hotend::for_tool(*tool).set_nozzle_target_temp(args.cleaning_temp);
    ScopeGuard restore_nozzle_target([&] {
        Hotend::for_tool(*tool).set_nozzle_target_temp(saved_nozzle_target);
    });

    if (!GcodeSuite::G28_no_parser(true, true, true, G28Flags { .only_if_needed = true })) {
        log_error(NozzleCleanerLite, "homing failed");
        return false;
    }

    move_to_machine_pos_z(current_machine_position().z + travel_clearance_mm, leave_feedrate);
    move_to_machine_pos_xy(touchpoint_xy.x, touchpoint_xy.y, dive_feedrate);

    // reach the cleaning temperature before touching down
    if (!thermalManager.wait_for_hotend(*tool, { .no_wait_for_cooling = false })) {
        log_error(NozzleCleanerLite, "temperature wait failed");
        return false;
    }

    const float probed_z = find_touchpoint_z();
    if (std::isnan(probed_z)) {
        return false;
    }
    log_info(NozzleCleanerLite, "Touchpoint surface at Z=%.3f", static_cast<double>(probed_z));

    move_to_machine_pos_z(probed_z + safe_above_surface_mm, approach_feedrate);

    // Start the zigzag somewhere new each run so consecutive cleans do not retrace the same track.
    ZigZag zigzag { cleaner_pad, crossings_per_stroke,
        static_cast<float>(ticks_ms() % 1000) / 2000.0f };

    // Safely move from the touchpoint onto the pad, at the zigzag's own starting point rather than the pad centre
    const auto zigzag_start = zigzag.start();
    move_to_pad(zigzag_start.along, zigzag_start.across, approach_feedrate);
    move_to_machine_pos_z(probed_z + wipe_above_surface_mm, dive_feedrate);

    const float saved_travel_acceleration = planner.user_settings.travel_acceleration;
    {
        auto s = planner.user_settings;
        s.travel_acceleration = rub_acceleration;
        planner.apply_settings(s);
    }

    // few fast cycles, then a couple of slower ones to finish cleanly.
    for (uint8_t i = 0; i < rub_cycles_fast; ++i) {
        rub_stroke(zigzag, cleaner_pad.along_far, rub_feedrate_fast);
        rub_stroke(zigzag, cleaner_pad.along_near, rub_feedrate_fast);
    }
    for (uint8_t i = 0; i < rub_cycles_slow; ++i) {
        rub_stroke(zigzag, cleaner_pad.along_far, rub_feedrate_slow);
        rub_stroke(zigzag, cleaner_pad.along_near, rub_feedrate_slow);
    }

    {
        auto s = planner.user_settings;
        s.travel_acceleration = saved_travel_acceleration;
        planner.apply_settings(s);
    }

    const uint8_t saved_fan_speed = thermalManager.get_print_fan_speed();
    ScopeGuard restore_fan_speed([&] {
        thermalManager.set_print_fan_speed(saved_fan_speed);
    });
    if (args.cooldown) {
        // Start cooling down already on the way back to the touchpoint
        Hotend::for_tool(*tool).set_nozzle_target_temp(rest_temp);
        thermalManager.set_print_fan_speed(255);
    }

    // Retreat back over the touchpoint
    move_to_machine_pos_z(probed_z + travel_clearance_mm, leave_feedrate);
    move_to_machine_pos_xy(touchpoint_xy.x, touchpoint_xy.y, leave_feedrate);

    if (!args.cooldown) {
        return true;
    }

    move_to_machine_pos_z(probed_z, dive_feedrate);

    ScopeGuard leave_touchpoint([&] {
        move_to_machine_pos_z(probed_z + final_clearance_mm, leave_feedrate);
    });

    if (!thermalManager.wait_for_hotend(*tool, { .no_wait_for_cooling = false })) {
        log_error(NozzleCleanerLite, "cooling failed");
        return false;
    }

    if (args.keep_target) {
        restore_nozzle_target.disarm();
    }
    return true;
}

} // namespace nozzle_cleaner_lite
