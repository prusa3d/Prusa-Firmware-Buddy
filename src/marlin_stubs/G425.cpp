/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @file
 */

#include <algorithm>
#include <array>
#include <span>
#include <stdint.h>
#include <sys/types.h>
#include <limits>

#include "core/types.h"
#include "metric.h"

#include "../../Marlin.h"

#include "../gcode.h"
#include "inc/Conditionals_LCD.h"

#if ENABLED(BACKLASH_GCODE)
    #include "../../feature/backlash.h"
#endif

#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../module/tool_change.h"
#include "../../module/endstops.h"
#include "../../module/prusa/homing_corexy.hpp"
#include "../../feature/bedlevel/bedlevel.h"
#include "Marlin/src/gcode/gcode.h"
#include "../../module/stepper.h"

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "loadcell.hpp"
    #include "../../module/prusa/toolchanger.h"
    #include "../../module/probe.h"
#endif

#if ENABLED(CRASH_RECOVERY)
    #include "src/feature/prusa/crash_recovery.hpp"
#endif

#include <bsod_gui.hpp>
#include <marlin_server.hpp>
#include <center_approx.hpp>

/**
 * G425 backs away from the calibration object by various distances
 * depending on the confidence level:
 *
 *   UNKNOWN   - No real notion on where the calibration object is on the bed
 *   UNCERTAIN - Measurement may be uncertain due to backlash
 *   CERTAIN   - Measurement obtained with backlash compensation
 */

#ifndef CALIBRATION_MEASUREMENT_UNKNOWN
    #define CALIBRATION_MEASUREMENT_UNKNOWN 5.0f // mm
#endif
#ifndef CALIBRATION_MEASUREMENT_UNCERTAIN
    #define CALIBRATION_MEASUREMENT_UNCERTAIN 1.0f // mm
#endif
#ifndef CALIBRATION_MEASUREMENT_CERTAIN
    #define CALIBRATION_MEASUREMENT_CERTAIN 0.5f // mm
#endif

#define NUM_Z_MEASUREMENTS 20

#define HAS_X_CENTER BOTH(CALIBRATION_MEASURE_LEFT, CALIBRATION_MEASURE_RIGHT)
#define HAS_Y_CENTER BOTH(CALIBRATION_MEASURE_FRONT, CALIBRATION_MEASURE_BACK)

#if ENABLED(ARC_SUPPORT)
void plan_arc(const xyze_pos_t &, const ab_float_t &, const bool, const uint8_t);
#else
    #error "G425 requires ARC_SUPPORT"
#endif

namespace {

// Absolute tool center - an input for offset computation [mm]
METRIC_DEF(metric_center, "g425_cen", METRIC_VALUE_CUSTOM, 100, METRIC_ENABLED);
// Tool offset relative to the first tool - result of the tool offset calibration [mm]
METRIC_DEF(metric_offset, "g425_off", METRIC_VALUE_CUSTOM, 100, METRIC_ENABLED);
// Raw XY probe [mm]
METRIC_DEF(metric_xy_raw_hit, "g425_rxy", METRIC_VALUE_CUSTOM, 100, METRIC_ENABLED);
// Verified XY probe - two raw probes agree on position [mm]
METRIC_DEF(metric_xy_hit, "g425_xy", METRIC_VALUE_CUSTOM, 100, METRIC_ENABLED);
// Raw Z probe [mm]
METRIC_DEF(metric_z_raw_hit, "g425_rz", METRIC_VALUE_CUSTOM, 100, METRIC_ENABLED);
// Averaged Z probe - N raw probes averaged [mm]
METRIC_DEF(metric_z_hit, "g425_z", METRIC_VALUE_CUSTOM, 100, METRIC_ENABLED);
// Max deviation
METRIC_DEF(metric_xy_dev, "g425_xy_dev", METRIC_VALUE_FLOAT, 100, METRIC_ENABLED);

constexpr xyz_float_t dimensions { { CALIBRATION_OBJECT_DIMENSIONS } };
constexpr xy_float_t nod = { { { CALIBRATION_NOZZLE_OUTER_DIAMETER, CALIBRATION_NOZZLE_OUTER_DIAMETER } } };
constexpr xyz_pos_t true_center { { CALIBRATION_OBJECT_CENTER } };
constexpr xyz_pos_t true_top_center = { { { .x = true_center.x,
    .y = true_center.y,
    .z = dimensions.z } } };

constexpr float PROBE_Z_BORE_MM { 1 };
constexpr auto PROBE_Z_UNCERTAIN_DIST_MM { 5 };
constexpr auto PROBE_Z_CERTAIN_DIST_MM { 1 };
constexpr float PIN_DIAMETER_MM { 6 };
constexpr float PROBE_XY_TIGHT_DIST_MM { PIN_DIAMETER_MM / 2 + 3 };
constexpr float PROBE_XY_CERTAIN_DIST_MM { PROBE_XY_TIGHT_DIST_MM + 1 };
constexpr float PROBE_XY_UNCERTAIN_DIST_MM { PROBE_XY_CERTAIN_DIST_MM + 1 };
constexpr feedRate_t PROBE_FEEDRATE_MMS { 3 };
constexpr feedRate_t INTERPROBE_FEEDRATE_MMS { 25 };
constexpr auto NUM_PROBE_TRIES { 5 }; // Keep low, the noise is not random, but rather the measurement jumps a few values
constexpr auto PROBE_ALLOWED_ERROR { 0.03f };
constexpr auto NUM_PROBE_SAMPLES { 2 };
constexpr auto PROBE_FAIL_THRESHOLD_MM { 3 };
constexpr auto XY_ACCELERATION_MMSS { 500 };
constexpr auto RESONANCE_DAMPER_WAIT_MS { 500 };
constexpr auto MIN_TRAVELED_DISTANCE_MM { 0.1f };
constexpr auto MAX_DEVIATION_MM { 0.2f };

struct measurements_t {
    xyz_pos_t obj_center = true_top_center; // Non-static must be assigned from xyz_pos_t
    xyz_float_t pos_error;
    xy_float_t nozzle_outer_dimension = nod;
};

/// Center find phase
// Center probing requires an approximate center as input. In order to increase precision the center
// is probed several times starting from hardcoded center. As the center position is refined the probe
// algorithms optimize retractions and increase number of hits in later phases.
enum class Phase : uint8_t {
    first,
    second,
    final,
    _count
};

#define TEMPORARY_SOFT_ENDSTOP_STATE(enable) REMEMBER(tes, soft_endstops_enabled, enable);

#if ENABLED(BACKLASH_GCODE)
    #define TEMPORARY_BACKLASH_CORRECTION(value) REMEMBER(tbst, backlash.correction, value)
#else
    #define TEMPORARY_BACKLASH_CORRECTION(value)
#endif

#if ENABLED(BACKLASH_GCODE) && defined(BACKLASH_SMOOTHING_MM)
    #define TEMPORARY_BACKLASH_SMOOTHING(value) REMEMBER(tbsm, backlash.smoothing_mm, value)
#else
    #define TEMPORARY_BACKLASH_SMOOTHING(value)
#endif

/// Limit max acceleration to a value, restore old value when destroyed
class AccelerationLimiter {
public:
    AccelerationLimiter(const float max_acceleration_mmss)
        : previous_x(planner.settings.max_acceleration_mm_per_s2[X_AXIS])
        , previous_y(planner.settings.max_acceleration_mm_per_s2[Y_AXIS]) {
        planner.set_max_acceleration(X_AXIS | Y_AXIS, max_acceleration_mmss);
    }
    ~AccelerationLimiter() {
        planner.set_max_acceleration(X_AXIS, previous_x);
        planner.set_max_acceleration(Y_AXIS, previous_y);
    }

private:
    const float previous_x, previous_y;
};

inline void calibration_move() {
    do_blocking_move_to(current_position, MMM_TO_MMS(CALIBRATION_FEEDRATE_TRAVEL));
}

inline void wait_ms(const uint32_t duration_ms) {
    const uint32_t point = ticks_ms();
    while (ticks_ms() - point < duration_ms) {
        idle(true);
    }
}

#if HOTENDS > 1
inline void set_nozzle([[maybe_unused]] measurements_t &m, const uint8_t extruder) {
    if (extruder != active_extruder) {
        tool_change(extruder);
    }
}
#endif

#if HAS_HOTEND_OFFSET
inline void normalize_hotend_offsets() {
    for (uint8_t e = 1; e < HOTENDS; e++) {
        hotend_offset[e] -= hotend_offset[0];
    }
    hotend_offset[0].reset();
    hotend_offset[PrusaToolChanger::MARLIN_NO_TOOL_PICKED].reset(); // Avoid offset on no tool
}
#endif

/// Return one of evenly distributed position on circle
xy_pos_t pos_on_circle(float radius, int idx, int total_points) {
    float goniom_dist = (static_cast<float>(idx) / static_cast<float>(total_points)) * 2 * static_cast<float>(M_PI);
    return { { { std::cos(goniom_dist) * radius, std::sin(goniom_dist) * radius } } };
}

// This function requires normalize_hotend_offsets() to be called
inline void report_hotend_offsets() {
    for (uint8_t e = 1; e < HOTENDS; e++) {
        SERIAL_ECHOLNPAIR("T", int(e), " Hotend Offset X", hotend_offset[e].x, " Y", hotend_offset[e].y, " Z", hotend_offset[e].z);
    }
}

xy_pos_t closest_point_on_circle(const xy_pos_t point, const xy_pos_t center, const float radius) {
    const float distance_factor = sqrt(pow(point.x - center.x, 2) + pow(point.y - center.y, 2));

    if (distance_factor == 0) {
        return { { { .x = center.x + radius,
            .y = center.y } } };
    }

    return { { { .x = center.x + radius * (point.x - center.x) / distance_factor,
        .y = center.y + radius * (point.y - center.y) / distance_factor } } };
}

void go_to_safe_height() {
    current_position.z = true_top_center.z + PROBE_Z_UNCERTAIN_DIST_MM;
    line_to_current_position(INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

void go_to_safety_circle(const xyz_pos_t center, const float radius) {
    current_position.set(closest_point_on_circle(current_position, center, radius));
    line_to_current_position(INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

xyz_pos_t initial_position(const xyz_pos_t center, const float angle, const float radius) {
    return { { { .x = center.x + cos(angle) * radius,
        .y = center.y + sin(angle) * radius,
        .z = center.z - PROBE_Z_BORE_MM } } };
}

void go_to_initial(const xyz_pos_t center, const float angle, const float radius) {
    const xyz_pos_t initial = initial_position(center, angle, radius);
    const xy_pos_t current = current_position;

    if (current != initial) {
        feedrate_mm_s = INTERPROBE_FEEDRATE_MMS;
        plan_arc(initial, { { { .x = center.x - current.x, .y = center.y - current.y } } }, false, 0);
    }
    planner.synchronize();

    current_position.z = initial.z;
    line_to_current_position(INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

xy_pos_t probe_xy(const xyz_pos_t center, const float angle, const uint8_t tool, const Phase phase) {
    // Wait for movements to finish
    planner.synchronize();

    // Mark initial position
    xyze_long_t initial_pos_msteps = planner.get_position_msteps();
    xyze_pos_t initial_mm = current_position;

    // Setup probe for XY endstop
    loadcell.set_xy_endstop(true);

    // Wait for resonance to damper and tare
    wait_ms(RESONANCE_DAMPER_WAIT_MS);
    loadcell.WaitBarrier(); // Sync samples before tare
    loadcell.Tare(Loadcell::TareMode::Continuous);

    if (loadcell.GetXYEndstop()) {
        // This is hopefully rare situation when the loadcell data are totally wrong. If we know this happens
        // and why it happens we should add a red screen with appropriate text.
        bsod("XY probe triggered");
    }

    // Expect pin hit
    endstops.enable_xy_probe(true);
#if ENABLED(CRASH_RECOVERY)
    crash_s.deactivate();
#endif

    // Go to center
    do_blocking_move_to_xy(center.x, center.y, PROBE_FEEDRATE_MMS);

    // No longer expecting pin hit
    const bool reached = endstops.trigger_state();
#if ENABLED(CRASH_RECOVERY)
    crash_s.activate();
#endif
    loadcell.set_xy_endstop(false);
    endstops.enable_xy_probe(false);

    // Something is terribly wrong, maybe the nozzle is already being bend, bail out.
    if (!reached) {
        fatal_error(ErrCode::ERR_MECHANICAL_PIN_NOT_REACHED);
    }

    // Get hit position
    endstops.hit_on_purpose();
    planner.reset_position();
    xyze_long_t hit_steps = { { { stepper.position(A_AXIS), stepper.position(B_AXIS),
        stepper.position(C_AXIS), stepper.position(E_AXIS) } } };
    xyze_pos_t hit_mm;
    corexy_ab_to_xyze(hit_steps, hit_mm);

    // Discard result if not moved enough to reach the pin (probe triggered too early?)
    if ((hit_mm - initial_mm).magnitude() < MIN_TRAVELED_DISTANCE_MM) {
        hit_mm.reset();
    }

    // Return to initial
    planner._buffer_msteps_raw(initial_pos_msteps, initial_mm, INTERPROBE_FEEDRATE_MMS, active_extruder);
    planner.synchronize();
    current_position = initial_mm;

    metric_record_custom(
        &metric_xy_raw_hit,
        ",t=%u,p=%u,a=%.3f x=%.3f,y=%.3f",
        tool,
        ftrstd::to_underlying(phase),
        static_cast<double>(angle),
        static_cast<double>(hit_mm.x),
        static_cast<double>(hit_mm.y));

    return hit_mm;
}

xy_pos_t synthetic_probe(const xyz_pos_t center, const float angle) {
    return { { { .x = center.x + (PIN_DIAMETER_MM / 2 + PROBE_Z_BORE_MM) * cos(angle),
        .y = center.y + (PIN_DIAMETER_MM / 2 + PROBE_Z_BORE_MM) * sin(angle) } } };
}

xy_pos_t probe_xy_verify(const xyz_pos_t center, const float angle, const float probe_distance, const uint8_t tool, const Phase phase) {
    go_to_safety_circle(center, probe_distance);
    go_to_initial(center, angle, probe_distance);

    // Take all samples
    std::array<xy_pos_t, NUM_PROBE_SAMPLES> hits;
    for (auto &hit : hits) {
        hit = probe_xy(center, angle, tool, phase);
    }

    for (uint i = 0; i < NUM_PROBE_TRIES; ++i) {
        // Compute position from hits
        xy_pos_t pos {};
        for (auto &hit : hits) {
            pos += hit;
        }
        pos = pos / static_cast<int>(hits.size());

        // Compute sum of hit distances from position
        float distance = 0;
        for (auto &hit : hits) {
            auto dist = (pos - hit).magnitude();
            if (dist > distance) {
                distance = dist;
            }
            if (!hit.magnitude()) {
                distance = std::numeric_limits<float>::infinity();
            }
        }

        // Check samples consistency
        if (distance > PROBE_ALLOWED_ERROR) {
            // Redo one of the samples and try again
            hits[i % hits.size()] = probe_xy(center, angle, tool, phase);
            continue;
        }

        // Samples are consistent
        metric_record_custom(
            &metric_xy_hit,
            ",t=%u,p=%u,a=%.3f x=%.3f,y=%.3f",
            tool,
            ftrstd::to_underlying(phase),
            static_cast<double>(angle),
            static_cast<double>(pos.x),
            static_cast<double>(pos.y));

        const float exp_dist = (pos - synthetic_probe(center, angle)).magnitude();
        if (exp_dist > PROBE_FAIL_THRESHOLD_MM) {
            fatal_error(ErrCode::ERR_MECHANICAL_XY_POSITION_INVALID, static_cast<double>(exp_dist), static_cast<double>(PROBE_FAIL_THRESHOLD_MM));
        }

        return pos;
    }

    fatal_error(ErrCode::ERR_MECHANICAL_XY_PROBE_UNSTABLE);

    return hits[0];
}

/// Probe in Z, first in the middle then it does circle around center of the pin, just to distribute the probes over larger area to minimize errors
float probe_z(const xyz_pos_t position, float uncertainty, const int num_measurements, const uint8_t tool, const Phase phase) {
    constexpr xyz_float_t dimensions = { { CALIBRATION_OBJECT_DIMENSIONS } };

    // radius of circle that we are probing around for more variety
    constexpr float circle_radius = std::min(dimensions.x, dimensions.y) / 4;

    // Move to safe clearance above calibration object first
    float top_expected_position = position.z;
    current_position.z = top_expected_position + uncertainty;
    calibration_move();

    float sum = 0;
    for (int i = 0; i < num_measurements; i++) {
        xy_pos_t offset = { { { 0, 0 } } };
        if (i > 0) {
            offset = pos_on_circle(circle_radius, i - 1, num_measurements - 1);
        }

        // Move to the position where we probe
        current_position = xy_pos_t(position) + offset;
        current_position.z = top_expected_position + uncertainty;
        calibration_move();

        float measurement = probe_here(top_expected_position);
        if (std::isnan(measurement)) {
            fatal_error(ErrCode::ERR_MECHANICAL_PIN_NOT_REACHED);
        }
        SERIAL_ECHOPAIR_F("Probe: ", static_cast<double>(measurement));
        SERIAL_EOL();
        metric_record_custom(
            &metric_z_raw_hit,
            ",t=%u,p=%u x=%.3f,y=%.3f,z=%.3f",
            tool,
            ftrstd::to_underlying(phase),
            static_cast<double>(current_position.x),
            static_cast<double>(current_position.y),
            static_cast<double>(measurement));

        // now that we have first probe, update top position and uncertainty to speed-up further probes
        top_expected_position = measurement;
        uncertainty = 1;

        sum += measurement;
    }

    float measurement_avg = sum / num_measurements;
    SERIAL_ECHOPAIR_F("Average: ", static_cast<double>(measurement_avg));
    SERIAL_EOL();

    metric_record_custom(
        &metric_z_hit,
        ",t=%u,p=%u,x=%.3f,y=%.3f z=%.3f",
        tool,
        static_cast<unsigned>(phase),
        static_cast<double>(current_position.x),
        static_cast<double>(current_position.y),
        static_cast<double>(measurement_avg));

    return measurement_avg;
}

/// Issue a warning if a point deviates too much from the circle
void check_deviation(const xy_pos_t &center, std::span<const xy_pos_t> points) {
    // Compute average radius
    float radius = 0;
    for (const xy_pos_t &p : points) {
        radius += (center - p).magnitude();
    }
    radius /= points.size();

    // Compute maximum deviation of point-center distance from average radius
    float max_dev = 0;
    for (const xy_pos_t &p : points) {
        max_dev = max(max_dev, (center - p).magnitude() - radius);
    }

    // Issue warning if maximum deviation is above threshold
    if (max_dev > MAX_DEVIATION_MM) {
        marlin_server::set_warning(WarningType::NozzleDoesNotHaveRoundSection);
    }
    metric_record_float(&metric_xy_dev, max_dev);
}

const xyz_pos_t get_single_xyz_center(const xyz_pos_t initial, const uint8_t tool, const Phase phase) {
    static constexpr uint8_t PHASE_XY_HITS[ftrstd::to_underlying(Phase::_count)] = { 3, 3, 12 };
    static constexpr uint8_t PHASE_Z_HITS[ftrstd::to_underlying(Phase::_count)] = { 1, 0, NUM_Z_MEASUREMENTS };
    static constexpr float PHASE_Z_UNCERTAINTY[ftrstd::to_underlying(Phase::_count)] = { PROBE_XY_UNCERTAIN_DIST_MM, PROBE_Z_UNCERTAIN_DIST_MM, PROBE_Z_CERTAIN_DIST_MM };
    xyz_pos_t start = initial;

    // Get Z
    if (PHASE_Z_HITS[ftrstd::to_underlying(phase)]) {
        start.z = probe_z(initial, PHASE_Z_UNCERTAINTY[ftrstd::to_underlying(phase)], PHASE_Z_HITS[ftrstd::to_underlying(phase)], tool, phase);
    }

    // Get XY
    AccelerationLimiter al(XY_ACCELERATION_MMSS);
    static constexpr uint8_t MAX_HITS = *std::max_element(std::begin(PHASE_XY_HITS), std::end(PHASE_XY_HITS));
    std::array<xy_pos_t, MAX_HITS> max_hits;
    std::span<xy_pos_t> hits(max_hits.begin(), PHASE_XY_HITS[ftrstd::to_underlying(phase)]);
    for (uint hit_no = 0; xy_pos_t & hit : hits) {
        hit = probe_xy_verify(start, 2 * PI / hits.size() * hit_no++, PROBE_XY_UNCERTAIN_DIST_MM, tool, phase);
    }
    xyz_pos_t center = approximate_center(hits);
    center.z = start.z;

    if (phase == Phase::final) {
        check_deviation(center, hits);
    }

    return center;
}

const xyz_pos_t get_xyz_center(const uint8_t tool) {

    // Enable loadcell high precision across the entire procedure to prime the noise filters
    auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell);

    xyz_pos_t center = true_top_center;
    for (Phase phase = Phase::first; phase != Phase::_count; phase = Phase(ftrstd::to_underlying(phase) + 1)) {
        center = get_single_xyz_center(center, tool, phase);
    }

    go_to_safe_height();

    return center;
}

inline void update_measurements(measurements_t &m, const AxisEnum axis) {
#if HAS_HOTEND_OFFSET
    hotend_currently_applied_offset[axis] += m.pos_error[axis];
#endif
    m.obj_center[axis] = true_top_center[axis];
    m.pos_error[axis] = 0;
}

/**
 * Probe around the calibration object. Adjust the position and toolhead offset
 * using the deviation from the known position of the calibration object.
 *
 *   m              in/out - Measurement record, updated with new readings
 *   extruder       in     - What extruder to probe
 *
 * Prerequisites:
 *    - Call calibrate_backlash() beforehand for best accuracy
 */
inline void calibrate_toolhead(measurements_t &m, const uint8_t extruder) {
    TEMPORARY_BACKLASH_CORRECTION(all_on);
    TEMPORARY_BACKLASH_SMOOTHING(0.0f);

#if HOTENDS > 1
    set_nozzle(m, extruder);
#else
    UNUSED(extruder);
#endif

    const xyz_pos_t center = get_xyz_center(extruder);
    m.obj_center = center;
    m.pos_error = true_top_center - center;

// Adjust the hotend offset
#if HAS_HOTEND_OFFSET
    #if HAS_X_CENTER
    hotend_offset[extruder].x += m.pos_error.x;
    #endif
    #if HAS_Y_CENTER
    hotend_offset[extruder].y += m.pos_error.y;
    #endif
    hotend_offset[extruder].z += m.pos_error.z;
    normalize_hotend_offsets();
#endif

// Update measurements
#if HAS_X_CENTER
    update_measurements(m, X_AXIS);
#endif
#if HAS_Y_CENTER
    update_measurements(m, Y_AXIS);
#endif
    update_measurements(m, Z_AXIS);
}

/**
 * Probe around the calibration object for all toolheads, adjusting the coordinate
 * system for the first nozzle and the nozzle offset for subsequent nozzles.
 *
 *   m              in/out - Measurement record, updated with new readings
 *   uncertainty    in     - How far away from the object to begin probing
 */
inline void calibrate_all_toolheads(measurements_t &m) {
    TEMPORARY_BACKLASH_CORRECTION(all_on);
    TEMPORARY_BACKLASH_SMOOTHING(0.0f);

    HOTEND_LOOP() {
#if ENABLED(PRUSA_TOOLCHANGER)
        if (!prusa_toolchanger.getTool(e).is_enabled()) {
            continue;
        }
#endif
        calibrate_toolhead(m, e);
    }

#if HAS_HOTEND_OFFSET
    normalize_hotend_offsets();
    #if ENABLED(PRUSA_TOOLCHANGER)
    prusa_toolchanger.save_tool_offsets();
    #endif
#endif
}

/**
 * Perform a full auto-calibration routine:
 *
 *   1) For each nozzle, touch top and sides of object to determine object position and
 *      nozzle offsets. Do a fast but rough search over a wider area.
 *   2) With the first nozzle, touch top and sides of object to determine backlash values
 *      for all axis (if BACKLASH_GCODE is enabled)
 */
inline void calibrate_all() {
    measurements_t m;

#if HAS_HOTEND_OFFSET
    reset_hotend_offsets();
#endif

    TEMPORARY_BACKLASH_CORRECTION(all_on);
    TEMPORARY_BACKLASH_SMOOTHING(0.0f);

    // Calibrate all toolheads
    calibrate_all_toolheads(m);

#if ENABLED(BACKLASH_GCODE)
    calibrate_backlash(m);
#endif

    tool_change(prusa_toolchanger.MARLIN_NO_TOOL_PICKED, tool_return_t::no_return);
}

/**
 * Perform a full auto-calibration routine - simple implementation:
 *
 * This is simplified version that:
 * - un-applies, zeroes all offsets
 * - measures pin centers for all the pins
 * - computes new offsets
 */
inline void calibrate_all_simple() {
    // Disable E steppers to reduce noise on loadcell
    disable_e_steppers();

#if ENABLED(CRASH_RECOVERY)
    // Disable crash recovery. It would recover, but the measurement will be inaccurate anyway.
    Crash_Temporary_Deactivate ctd;
#endif

    // Reset planner state
    planner.synchronize();
    planner.reset_position();

    // Zero hotend offsets
    reset_hotend_offsets();
    hotend_currently_applied_offset = 0.f;

    // Measure centers
    std::array<xyz_pos_t, HOTENDS> centers;
    HOTEND_LOOP() {
#if ENABLED(PRUSA_TOOLCHANGER)
        if (!prusa_toolchanger.getTool(e).is_enabled()) {
            continue;
        }
#endif
        tool_change(e, tool_return_t::no_return);
        centers[e] = get_xyz_center(e);
        metric_record_custom(
            &metric_center,
            ",t=%u x=%.3f,y=%.3f,z=%.3f",
            e,
            static_cast<double>(centers[e].x),
            static_cast<double>(centers[e].y),
            static_cast<double>(centers[e].z));
    }

    // Pick zero offset tool to be sure no offset is applied on toolchange
    tool_change(prusa_toolchanger.MARLIN_NO_TOOL_PICKED, tool_return_t::no_return);

    // Apply the offset
    HOTEND_LOOP() {
        if (!prusa_toolchanger.getTool(e).is_enabled()) {
            continue;
        }
        // One might ask why the "-" in front of the centers[e].
        // Remember, when tool is bend +x it will find the object at the position -x.
        hotend_offset[e] = -centers[e];
    }
    normalize_hotend_offsets();

    // Check offsets
    HOTEND_LOOP() {
#if ENABLED(PRUSA_TOOLCHANGER)
        if (!prusa_toolchanger.getTool(e).is_enabled()) {
            hotend_offset[e].reset();
            continue;
        }
#endif

        if (hotend_offset[e].x < X_MIN_OFFSET || hotend_offset[e].x > X_MAX_OFFSET) {
            fatal_error(ErrCode::ERR_MECHANICAL_TOOL_OFFSET_OUT_OF_BOUNDS, e + 1, 'X', static_cast<double>(hotend_offset[e].x), static_cast<double>(X_MIN_OFFSET), static_cast<double>(X_MAX_OFFSET));
        }
        if (hotend_offset[e].y < Y_MIN_OFFSET || hotend_offset[e].y > Y_MAX_OFFSET) {
            fatal_error(ErrCode::ERR_MECHANICAL_TOOL_OFFSET_OUT_OF_BOUNDS, e + 1, 'Y', static_cast<double>(hotend_offset[e].y), static_cast<double>(Y_MIN_OFFSET), static_cast<double>(Y_MAX_OFFSET));
        }
        if (hotend_offset[e].z < Z_MIN_OFFSET || hotend_offset[e].z > Z_MAX_OFFSET) {
            fatal_error(ErrCode::ERR_MECHANICAL_TOOL_OFFSET_OUT_OF_BOUNDS, e + 1, 'Z', static_cast<double>(hotend_offset[e].z), static_cast<double>(Z_MIN_OFFSET), static_cast<double>(Z_MAX_OFFSET));
        }
    }
    prusa_toolchanger.save_tool_offsets();

    HOTEND_LOOP() {
        if (!prusa_toolchanger.getTool(e).is_enabled()) {
            continue;
        }
        osDelay(100); // Some time to send the metric before sending the next one
        metric_record_custom(
            &metric_offset,
            ",t=%u x=%.3f,y=%.3f,z=%.3f",
            e,
            static_cast<double>(hotend_offset[e].x),
            static_cast<double>(hotend_offset[e].y),
            static_cast<double>(hotend_offset[e].z));
    }
}

} // anonymous namespace

/**
 * G425: Perform calibration with calibration object.
 *
 *   B           - Perform calibration of backlash only.
 *   T<extruder> - Perform calibration of toolhead only.
 *   V           - Probe object and print position, error, backlash and hotend offset.
 *   U           - Uncertainty, how far to start probe away from the object (mm)
 *
 *   no args     - Perform entire calibration sequence (backlash + position on all toolheads)
 */
void GcodeSuite::G425() {
    TEMPORARY_SOFT_ENDSTOP_STATE(false);
    TEMPORARY_BED_LEVELING_STATE(false);

    phase_stepping::EnsureDisabled ps_disabler {};

    if (axis_unhomed_error()) {
        return;
    }

    measurements_t m;

    if (parser.seen('T')) {
        calibrate_toolhead(m, parser.has_value() ? parser.value_int() : active_extruder);
    } else {
        // calibrate_all();
        calibrate_all_simple();
    }
}
