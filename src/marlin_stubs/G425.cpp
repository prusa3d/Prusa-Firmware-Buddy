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
 */

#include <span>

#include "../../Marlin.h"

#if ENABLED(CALIBRATION_GCODE)

    #include "../gcode.h"

    #if ENABLED(BACKLASH_GCODE)
        #include "../../feature/backlash.h"
    #endif

    #include "../../module/motion.h"
    #include "../../module/planner.h"
    #include "../../module/tool_change.h"
    #include "../../module/endstops.h"
    #include "../../feature/bedlevel/bedlevel.h"
    #include "Marlin/src/gcode/gcode.h"
    #include "../../module/stepper.h"

    #if ENABLED(PRUSA_TOOLCHANGER)
        #include "loadcell.h"
        #include "../../module/prusa/toolchanger.h"
        #include "../../module/probe.h"
    #endif

    #if ENABLED(CRASH_RECOVERY)
        #include "src/feature/prusa/crash_recovery.h"
    #endif

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

static constexpr xyz_float_t dimensions CALIBRATION_OBJECT_DIMENSIONS;
static constexpr xy_float_t nod = { CALIBRATION_NOZZLE_OUTER_DIAMETER, CALIBRATION_NOZZLE_OUTER_DIAMETER };
static constexpr xyz_pos_t true_center CALIBRATION_OBJECT_CENTER;
static constexpr xyz_pos_t true_top_center = { { { .x = true_center.x,
    .y = true_center.y,
    .z = dimensions.z } } };

static constexpr float PROBE_Z_BORE_MM { 1 };
static constexpr auto PROBE_Z_UNCERTAIN_DIST_MM { 5 };
static constexpr auto PROBE_Z_CERTAIN_DIST_MM { 1 };
static constexpr float PIN_DIAMETER_MM { 6 };
static constexpr double TIGHT_RADIUS_MM { PIN_DIAMETER_MM / 2 + 3 };
static constexpr double FAST_RADIUS_MM { TIGHT_RADIUS_MM + 1 };
static constexpr double SAFETY_RADIUS_MM { FAST_RADIUS_MM + 1 };
static constexpr auto PROBE_FEEDRATE_MMS { 3 };
static constexpr auto INTERPROBE_FEEDRATE_MMS { 50 };
static constexpr float RETREAT_MM { 1 };
static constexpr auto NUM_PROBE_TRIES { 20 };
static constexpr auto PROBE_ALLOWED_ERROR { 0.02f };
static constexpr auto NUM_PROBE_SAMPLES { 2 };
static constexpr auto PROBE_FAIL_THRESHOLD_MM { 5 };

struct measurements_t {
    xyz_pos_t obj_center = true_top_center; // Non-static must be assigned from xyz_pos_t
    xyz_float_t pos_error;
    xy_float_t nozzle_outer_dimension = nod;
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

    #if ENABLED(ARC_SUPPORT)
void plan_arc(const xyze_pos_t &cart, const ab_float_t &offset, const uint8_t clockwise);
    #endif

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
inline void set_nozzle(measurements_t &m, const uint8_t extruder) {
    if (extruder != active_extruder) {
        tool_change(extruder);
    }
}
    #endif

    #if HAS_HOTEND_OFFSET
inline void normalize_hotend_offsets() {
    for (uint8_t e = 1; e < HOTENDS; e++)
        hotend_offset[e] -= hotend_offset[0];
    hotend_offset[0].reset();
    hotend_offset[PrusaToolChanger::MARLIN_NO_TOOL_PICKED].reset(); // Avoid offset on no tool
}
    #endif

inline bool read_calibration_pin() {
    #if HAS_CALIBRATION_PIN
    return (READ(CALIBRATION_PIN) != CALIBRATION_PIN_INVERTING);
    #elif ENABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
    return (READ(Z_MIN_PIN) != Z_MIN_ENDSTOP_INVERTING);
    #else
    return (READ(Z_MIN_PROBE_PIN) != Z_MIN_PROBE_ENDSTOP_INVERTING);
    #endif
}

/// Return one of evenly distributed position on circle
static xy_pos_t pos_on_circle(float radius, int idx, int total_points) {
    float goniom_dist = (static_cast<float>(idx) / static_cast<float>(total_points)) * 2 * static_cast<float>(M_PI);
    return { std::cos(goniom_dist) * radius, std::sin(goniom_dist) * radius };
}

// This function requires normalize_hotend_offsets() to be called
inline void report_hotend_offsets() {
    for (uint8_t e = 1; e < HOTENDS; e++)
        SERIAL_ECHOLNPAIR("T", int(e), " Hotend Offset X", hotend_offset[e].x, " Y", hotend_offset[e].y, " Z", hotend_offset[e].z);
}

static xy_pos_t closest_point_on_circle(const xy_pos_t point, const xy_pos_t center, const float radius) {
    const float distance_factor = sqrt(pow(point.x - center.x, 2) + pow(point.y - center.y, 2));

    if (distance_factor == 0) {
        return { { { .x = center.x + radius,
            .y = center.y } } };
    }

    return { { { .x = center.x + radius * (point.x - center.x) / distance_factor,
        .y = center.y + radius * (point.y - center.y) / distance_factor } } };
}

static void go_to_safe_height() {
    current_position.z = true_top_center.z + PROBE_Z_UNCERTAIN_DIST_MM;
    line_to_current_position(INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

static void go_to_safety_circle(const xyz_pos_t center, const float radius) {
    current_position.set(closest_point_on_circle(current_position, center, radius));
    line_to_current_position(INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

static xyz_pos_t initial_position(const xyz_pos_t center, const float angle, const float radius) {
    return { { { .x = center.x + cos(angle) * radius,
        .y = center.y + sin(angle) * radius,
        .z = center.z - PROBE_Z_BORE_MM } } };
}

static void go_to_initial(const xyz_pos_t center, const double angle, const double radius) {
    const xyz_pos_t initial = initial_position(center, angle, radius);
    const xy_pos_t current = current_position;

    if (current != initial) {
        plan_arc(initial, { { { .x = center.x - current.x, .y = center.y - current.y } } }, false);
    }
    planner.synchronize();

    current_position.z = initial.z;
    line_to_current_position(INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

static float point_distance(const xy_pos_t a, const xy_pos_t b) {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

static xy_pos_t countinuous_probe_inner(const xyz_pos_t center, const double angle) {
    // Wait for movements to finish
    planner.synchronize();

    // Wait for resonance to damper and setup probe
    auto enabler = Loadcell::HighPrecisionEnabler(loadcell);
    wait_ms(250);
    loadcell.analysis.Reset();
    loadcell.Tare();

    // Mark initial position
    const xy_pos_t initial = current_position;

    // Expect pin hit
    loadcell.set_xy_endstop(true);
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
        kill("Not reached pin");
    }

    // Fix position
    endstops.hit_on_purpose();
    set_current_from_steppers_for_axis(A_AXIS);
    set_current_from_steppers_for_axis(B_AXIS);
    sync_plan_position();
    const auto hit = current_position;

    // Return to initial
    current_position = initial;
    line_to_current_position(INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();

    return hit;
}

static xy_pos_t synthetic_probe(const xyz_pos_t center, const float angle) {
    return { { { .x = center.x + (PIN_DIAMETER_MM / 2 + PROBE_Z_BORE_MM) * cos(angle),
        .y = center.y + (PIN_DIAMETER_MM / 2 + PROBE_Z_BORE_MM) * sin(angle) } } };
}

static xy_pos_t countinuous_probe(const xyz_pos_t center, const double angle, bool fast = false) {
    go_to_safety_circle(center, fast ? FAST_RADIUS_MM : SAFETY_RADIUS_MM);
    go_to_initial(center, angle, fast ? FAST_RADIUS_MM : SAFETY_RADIUS_MM);

    std::array<xy_pos_t, NUM_PROBE_SAMPLES> hits = {
        countinuous_probe_inner(center, angle),
        countinuous_probe_inner(center, angle)
    };

    for (uint i = 0; i < NUM_PROBE_TRIES; ++i) {
        const float distance = point_distance(hits[0], hits[1]);
        if (distance < PROBE_ALLOWED_ERROR) {
            const xy_pos_t pos = (hits[0] + hits[1]) / 2;
            if (point_distance(pos, synthetic_probe(center, angle)) > PROBE_FAIL_THRESHOLD_MM) {
                kill("XY probe failed: invalid position");
            }
            return pos;
        }
        hits[i % hits.size()] = countinuous_probe_inner(center, angle);
    }

    kill("XY probe failed: unstable");
    return hits[0];
}

// Simple approx center as average
static const xy_pos_t approx_center(std::span<const xy_pos_t> points) {
    // Average as center
    xy_pos_t center = { { { .x = 0, .y = 0 } } };
    for (const xy_pos_t &point : points) {
        center += point;
    }
    center.x /= points.size();
    center.y /= points.size();

    return center;
}

/// Probe in Z, first in the middle then it does circle around center of the pin, just to distribute the probes over larger area to minimize errors
float probe_z(const xyz_pos_t position, float uncertainty, const int num_measurements) {
    constexpr xyz_float_t dimensions = CALIBRATION_OBJECT_DIMENSIONS;

    // radius of circle that we are probing around for more variety
    constexpr float circle_radius = std::min(dimensions.x, dimensions.y) / 4;

    // Move to safe clearance above calibration object first
    float top_expected_position = position.z;
    current_position.z = top_expected_position + uncertainty;
    calibration_move();

    float summ = 0;
    for (int i = 0; i < num_measurements; i++) {
        xy_pos_t offset = { 0, 0 };
        if (i > 0) {
            offset = pos_on_circle(circle_radius, i - 1, num_measurements - 1);
        }

        // Move to the position where we probe
        current_position = xy_pos_t(position) + offset;
        current_position.z = top_expected_position + uncertainty;
        calibration_move();

        float measurement = probe_here(top_expected_position);
        if (std::isnan(measurement)) {
            kill("PROBING ERROR", "Probe not successfull");
        }
        SERIAL_ECHOPAIR_F("Probe: ", measurement);
        SERIAL_EOL();

        // now that we have first probe, update top position and uncertanty to speed-up further probes
        top_expected_position = measurement;
        uncertainty = 1;

        summ += measurement;
    }

    float measurement_avg = summ / num_measurements;
    SERIAL_ECHOPAIR_F("Average: ", measurement_avg);
    SERIAL_EOL();

    return measurement_avg;
}

static const xyz_pos_t get_xyz_center(const uint phase) {
    // Go to start position
    go_to_safe_height();

    // First Z approximation
    const float initial_height = probe_z(true_top_center, PROBE_Z_UNCERTAIN_DIST_MM, 1);
    const xyz_pos_t start = { { { .x = true_top_center.x,
        .y = true_top_center.y,
        .z = initial_height } } };

    // First XY approximation
    std::array<xy_pos_t, 3> initial_hits;
    uint hit_no = 0;
    for (xy_pos_t &hit : initial_hits) {
        hit = countinuous_probe(start, 2 * PI / initial_hits.size() * hit_no++, false);
    }
    xyz_pos_t initial_center = approx_center(initial_hits);
    initial_center.z = start.z;

    // Second XY approximation
    std::array<xy_pos_t, 3> second_hits;
    uint second_hit_no = 0;
    for (xy_pos_t &hit : second_hits) {
        hit = countinuous_probe(initial_center, 2 * PI / second_hits.size() * second_hit_no++, true);
    }
    xyz_pos_t second_center = approx_center(second_hits);
    second_center.z = initial_center.z;

    // XYZ measurement
    std::array<xy_pos_t, 12> hits;
    for (uint i = 0; i < hits.size(); ++i) {
        hits[i] = countinuous_probe(second_center, 2 * PI / hits.size() * i, true);
    }
    xyz_pos_t center = approx_center(hits);
    center.z = initial_height;
    center.z = probe_z(center, PROBE_Z_CERTAIN_DIST_MM, NUM_Z_MEASUREMENTS);

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

    tool_change(prusa_toolchanger.MARLIN_NO_TOOL_PICKED, tool_return_t::no_move);
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
        tool_change(e);
        centers[e] = get_xyz_center(e);
    }

    // Pick zero offset tool to be sure no offset is applied on toolchange
    tool_change(prusa_toolchanger.MARLIN_NO_TOOL_PICKED, tool_return_t::no_move);

    // Apply the offset
    HOTEND_LOOP() {
        if (!prusa_toolchanger.getTool(e).is_enabled()) {
            continue;
        }
        // One might think why the "-" in front of the centers[e].
        // Remember, when tool is bend +x it will find the object at the position -x.
        hotend_offset[e] = -centers[e];
    }
    normalize_hotend_offsets();
    prusa_toolchanger.save_tool_offsets();
}

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

    if (axis_unhomed_error())
        return;

    measurements_t m;

    if (parser.seen('T'))
        calibrate_toolhead(m, parser.has_value() ? parser.value_int() : active_extruder);
    else
        // calibrate_all();
        calibrate_all_simple();
}

#endif // CALIBRATION_GCODE
