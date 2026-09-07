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

#include "G425.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <stdint.h>
#include <sys/types.h>
#include <limits>
#include <utils/variant_utils.hpp>
#include <common/mapi/acceleration_limiter.hpp>

#include "core/types.h"
#include "metric.h"

#include "../../Marlin.h"

#include "../gcode.h"
#include "inc/Conditionals_LCD.h"

#if ENABLED(BACKLASH_GCODE)
    // #error dead code found by automatic analyses (see BFW-5461)
    #include "../../feature/backlash.h"
#endif

#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../module/tool_change.h"
#include "../../module/endstops.h"
#include "../../module/prusa/corexy_transform.hpp"
#include "../../feature/bedlevel/bedlevel.h"
#include "../../feature/pressure_advance/pressure_advance_config.hpp"
#include "Marlin/src/gcode/gcode.h"
#include "../../module/stepper.h"

#include <option/has_crash_detection.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "loadcell.hpp"
    #include "../../module/prusa/toolchanger.h"
    #include "../../module/probe.h"
#endif

#if HAS_CRASH_DETECTION()
    #include "src/feature/prusa/crash_recovery.hpp"
#endif

#include <common/mapi/parking.hpp>
#include <bsod_gui.hpp>
#include <marlin_server.hpp>
#include <center_approx.hpp>
#include <printers.h>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>

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

#if PRINTER_IS_PRUSA_XL()

    #define CALIBRATION_FEEDRATE_TRAVEL 3000 // mm/m

    // The following parameter refers to the conical section of the nozzle tip.
    #define CALIBRATION_NOZZLE_OUTER_DIAMETER 2.0f // mm

    // The true location and dimension of the calibration pin on the bed.
    #define CALIBRATION_OBJECT_CENTER \
        { 180.0f, 180.0f, 4.5f } // mm
    #define CALIBRATION_OBJECT_DIMENSIONS \
        { 6.0f, 6.0f, 9.0f } // mm

    // Comment out any sides which are unreachable by the probe. For best
    // auto-calibration results, all sides must be reachable.
    #define CALIBRATION_MEASURE_RIGHT
    #define CALIBRATION_MEASURE_FRONT
    #define CALIBRATION_MEASURE_LEFT
    #define CALIBRATION_MEASURE_BACK

#else
    #error
#endif

#define HAS_X_CENTER BOTH(CALIBRATION_MEASURE_LEFT, CALIBRATION_MEASURE_RIGHT)
#define HAS_Y_CENTER BOTH(CALIBRATION_MEASURE_FRONT, CALIBRATION_MEASURE_BACK)

#if DISABLED(ARC_SUPPORT)
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
constexpr MachinePosXYZ true_center { { CALIBRATION_OBJECT_CENTER } };
constexpr MachinePosXYZ true_top_center = { { { .x = true_center.x,
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
    MachinePosXYZ obj_center = true_top_center; // Non-static must be assigned from MachinePosXYZ
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
    // #error dead code found by automatic analyses (see BFW-5461)
    #define TEMPORARY_BACKLASH_CORRECTION(value) REMEMBER(tbst, backlash.correction, value)
#else
    #define TEMPORARY_BACKLASH_CORRECTION(value)
#endif

#if ENABLED(BACKLASH_GCODE) && defined(BACKLASH_SMOOTHING_MM)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define TEMPORARY_BACKLASH_SMOOTHING(value) REMEMBER(tbsm, backlash.smoothing_mm, value)
#else
    #define TEMPORARY_BACKLASH_SMOOTHING(value)
#endif

inline void wait_ms(const uint32_t duration_ms) {
    const uint32_t point = ticks_ms();
    while (ticks_ms() - point < duration_ms) {
        idle(true);
    }
}

#if HOTENDS > 1
inline void set_nozzle([[maybe_unused]] measurements_t &m, const uint8_t extruder) {
    if (!stdext::holds_value(PhysicalToolIndex::currently_selected(), PhysicalToolIndex::from_raw(extruder))) {
        tool_change(PhysicalToolIndex::from_raw(extruder));
    }
}
#endif

#if HAS_HOTEND_OFFSET
inline void normalize_hotend_offsets() {
    const auto first_hotend_offset = hotend_offset[PhysicalToolIndex::from_raw(0)];
    for (auto tool : PhysicalToolIndex::all()) {
        hotend_offset[tool] -= first_hotend_offset;
    }
    [[maybe_unused]] const auto zero_offset = hotend_offset[PhysicalToolIndex::from_raw(0)];
    debug_assert(zero_offset.x == 0 && zero_offset.y == 0 && zero_offset.z == 0);
    hotend_offset[PrusaToolChanger::MARLIN_NO_TOOL_PICKED] = {}; // Avoid offset on no tool
}
#endif

/// Return one of evenly distributed position on circle
MachinePosXY pos_on_circle(float radius, int idx, int total_points) {
    float goniom_dist = (static_cast<float>(idx) / static_cast<float>(total_points)) * 2 * static_cast<float>(M_PI);
    return { { { std::cos(goniom_dist) * radius, std::sin(goniom_dist) * radius } } };
}

// This function requires normalize_hotend_offsets() to be called
inline void report_hotend_offsets() {
    for (auto tool : PhysicalToolIndex::all()) {
        const auto &offset = hotend_offset[tool];
        SERIAL_ECHOLNPAIR("T", static_cast<int>(tool.to_raw()), " Hotend Offset X", offset.x, " Y", offset.y, " Z", offset.z);
    }
}

MachinePosXY closest_point_on_circle(const MachinePosXY &point, const MachinePosXY &center, const float radius) {
    const float distance_factor = std::hypot(point.x - center.x, point.y - center.y);

    if (distance_factor == 0) {
        return {
            .x = center.x + radius,
            .y = center.y,
        };
    }

    return {
        .x = center.x + radius * (point.x - center.x) / distance_factor,
        .y = center.y + radius * (point.y - center.y) / distance_factor,
    };
}

void go_to_safe_height() {
    auto target = current_machine_position();
    target.z = true_top_center.z + PROBE_Z_UNCERTAIN_DIST_MM;
    line_to_machine_pos(target, INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

void go_to_safety_circle(const MachinePosXY &center, const float radius) {
    auto target = current_machine_position();
    target.set(closest_point_on_circle(target.xy(), center, radius));
    line_to_machine_pos(target, INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

MachinePosXYZ initial_position(const MachinePosXYZ &center, const float angle, const float radius) {
    return {
        .x = center.x + cos(angle) * radius,
        .y = center.y + sin(angle) * radius,
        .z = center.z - PROBE_Z_BORE_MM,
    };
}

void go_to_initial(const MachinePosXYZ &center, const float angle, const float radius) {
    MachinePosXYZE initial = current_machine_position();
    initial.set(initial_position(center, angle, radius));

    const MachinePosXY current = current_machine_position().xy();

    if (current != initial.xy()) {
        feedrate_mm_s = INTERPROBE_FEEDRATE_MMS;
        plan_arc(to_native_pos(initial), { { { .x = center.x - current.x, .y = center.y - current.y } } }, false, 0);
    }
    planner.synchronize();

    auto target = current_machine_position();
    target.z = initial.z;
    line_to_machine_pos(target, INTERPROBE_FEEDRATE_MMS);
    planner.synchronize();
}

MachinePosXY probe_xy(const MachinePosXYZ &center, const float angle, const uint8_t tool, const Phase phase) {
    const auto initial_mm = current_machine_position();

    // As we perform measurements, we need to ensure the current position has been reached first
    planner.buffer_line(initial_mm, PROBE_FEEDRATE_MMS, PhysicalToolIndex::currently_selected(), { .raw_block = true });
    planner.synchronize();

    // Mark initial position
    xyze_msteps_t initial_pos_msteps = planner.get_position_msteps();

    // Setup probe for XY endstop
    loadcell.set_xy_endstop(true);

    // Wait for resonance to damper and tare
    wait_ms(RESONANCE_DAMPER_WAIT_MS);
    {
        // Arm for tare + pin approach only; the return move at INTERPROBE_FEEDRATE_MMS
        // must not be quick-stoppable.
        Loadcell::ProbeSafetyArmer safetyArmer(loadcell);
        loadcell.WaitBarrier(); // Sync samples before tare
        loadcell.Tare(Loadcell::TareMode::Continuous);

        if (loadcell.GetXYEndstop()) {
            // This is hopefully rare situation when the loadcell data are totally wrong. If we know this happens
            // and why it happens we should add a red screen with appropriate text.
            bsod("XY probe triggered");
        }

        // Expect pin hit
        endstops.enable_xy_probe(true);
#if HAS_CRASH_DETECTION()
        crash_s.deactivate();
#endif

        // Go to center
        auto target = initial_mm;
        target.set(center.xy());
        line_to_machine_pos(target, PROBE_FEEDRATE_MMS);
        planner.synchronize();
    } // safetyArmer disarms here; return move runs unarmed

    // No longer expecting pin hit
    const bool reached = endstops.trigger_state();
#if HAS_CRASH_DETECTION()
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
    ab_steps_t hit_steps = { stepper.position(A_AXIS), stepper.position(B_AXIS) };
    xyze_pos_t hit_mm;
    corexy_ab_to_xyze(hit_steps, hit_mm);

    // Discard result if not moved enough to reach the pin (probe triggered too early?)
    if ((hit_mm - initial_mm).magnitude() < MIN_TRAVELED_DISTANCE_MM) {
        hit_mm = {};
    }

    // Return to initial
    planner._buffer_msteps(initial_pos_msteps, initial_mm, INTERPROBE_FEEDRATE_MMS, PhysicalToolIndex::currently_selected(), { .raw_block = true });
    planner.synchronize();
    set_current_position(to_native_pos(initial_mm));

    metric_record_custom(
        &metric_xy_raw_hit,
        ",t=%u,p=%u,a=%.3f x=%.3f,y=%.3f",
        tool,
        std::to_underlying(phase),
        static_cast<double>(angle),
        static_cast<double>(hit_mm.x),
        static_cast<double>(hit_mm.y));

    return hit_mm.xy();
}

MachinePosXY synthetic_probe(const MachinePosXYZ &center, const float angle) {
    return { { { .x = center.x + (PIN_DIAMETER_MM / 2 + PROBE_Z_BORE_MM) * cos(angle),
        .y = center.y + (PIN_DIAMETER_MM / 2 + PROBE_Z_BORE_MM) * sin(angle) } } };
}

MachinePosXY probe_xy_verify(const MachinePosXYZ &center, const float angle, const float probe_distance, const uint8_t tool, const Phase phase) {
    go_to_safety_circle(center.xy(), probe_distance);
    go_to_initial(center, angle, probe_distance);

    // Take all samples
    std::array<MachinePosXY, NUM_PROBE_SAMPLES> hits;
    for (auto &hit : hits) {
        hit = probe_xy(center, angle, tool, phase);
    }

    for (uint i = 0; i < NUM_PROBE_TRIES; ++i) {
        // Compute position from hits
        MachinePosXY pos {};
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
            std::to_underlying(phase),
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
float probe_z(const MachinePosXYZ &position, float uncertainty, const int num_measurements, const uint8_t tool, const Phase phase) {
    constexpr xyz_float_t dimensions = { { CALIBRATION_OBJECT_DIMENSIONS } };

    // radius of circle that we are probing around for more variety
    constexpr float circle_radius = std::min(dimensions.x, dimensions.y) / 4;

    // Move to safe clearance above calibration object first
    float top_expected_position = position.z;

    {
        auto target = current_machine_position();
        target.z = top_expected_position + uncertainty;
        line_to_machine_pos(target, MMM_TO_MMS(CALIBRATION_FEEDRATE_TRAVEL));
        planner.synchronize();
    }

    float sum = 0;
    for (int i = 0; i < num_measurements; i++) {
        MachinePosXY offset = { { { 0, 0 } } };
        if (i > 0) {
            offset = pos_on_circle(circle_radius, i - 1, num_measurements - 1);
        }

        // Move to the position where we probe
        {
            auto target = current_machine_position();
            target.set(position.xy() + offset);
            target.z = top_expected_position + uncertainty;
            line_to_machine_pos(target, MMM_TO_MMS(CALIBRATION_FEEDRATE_TRAVEL));
            planner.synchronize();
        }

        float measurement = probe_here(top_expected_position, TOTAL_PROBING);
        if (std::isnan(measurement)) {
            fatal_error(ErrCode::ERR_MECHANICAL_PIN_NOT_REACHED);
        }
        SERIAL_ECHOPAIR_F("Probe: ", static_cast<double>(measurement));
        SERIAL_EOL();
        metric_record_custom(
            &metric_z_raw_hit,
            ",t=%u,p=%u x=%.3f,y=%.3f,z=%.3f",
            tool,
            std::to_underlying(phase),
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
bool check_deviation(const MachinePosXY &center, std::span<const MachinePosXY> points) {
    // Compute average radius
    float radius = 0;
    for (const MachinePosXY &p : points) {
        radius += (center - p).magnitude();
    }
    radius /= points.size();

    // Compute maximum deviation of point-center distance from average radius
    float max_dev = 0;
    for (const MachinePosXY &p : points) {
        max_dev = max(max_dev, (center - p).magnitude() - radius);
    }

    // Issue warning if maximum deviation is above threshold
    if (max_dev > MAX_DEVIATION_MM) {
        return false;
    }
    metric_record_float(&metric_xy_dev, max_dev);
    return true;
}

const std::optional<MachinePosXYZ> get_single_xyz_center(const MachinePosXYZ &initial, const uint8_t tool, const Phase phase) {
    static constexpr uint8_t PHASE_XY_HITS[std::to_underlying(Phase::_count)] = { 3, 3, 12 };
    static constexpr uint8_t PHASE_Z_HITS[std::to_underlying(Phase::_count)] = { 1, 0, NUM_Z_MEASUREMENTS };
    static constexpr float PHASE_Z_UNCERTAINTY[std::to_underlying(Phase::_count)] = { PROBE_XY_UNCERTAIN_DIST_MM, PROBE_Z_UNCERTAIN_DIST_MM, PROBE_Z_CERTAIN_DIST_MM };
    MachinePosXYZ start = initial;

    // Get Z
    if (PHASE_Z_HITS[std::to_underlying(phase)]) {
        start.z = probe_z(initial, PHASE_Z_UNCERTAINTY[std::to_underlying(phase)], PHASE_Z_HITS[std::to_underlying(phase)], tool, phase);
    }

    // Get XY
    mapi::AccelerationLimiter al(XY_ACCELERATION_MMSS);
    static constexpr uint8_t MAX_HITS = *std::max_element(std::begin(PHASE_XY_HITS), std::end(PHASE_XY_HITS));
    std::array<MachinePosXY, MAX_HITS> max_hits;
    std::span<MachinePosXY> hits(max_hits.begin(), PHASE_XY_HITS[std::to_underlying(phase)]);
    for (uint hit_no = 0; MachinePosXY & hit : hits) {
        hit = probe_xy_verify(start, 2 * std::numbers::pi_v<float> / hits.size() * hit_no++, PROBE_XY_UNCERTAIN_DIST_MM, tool, phase);
    }
    MachinePosXYZ center = MachinePosXYZ(approximate_center(hits));
    center.z = start.z;

    if (phase == Phase::final) {
        if (!check_deviation(center.xy(), hits)) {
            return std::nullopt;
        }
    }

    return center;
}

const std::optional<MachinePosXYZ> get_xyz_center(const uint8_t tool) {

    // Enable loadcell high precision across the entire procedure to prime the noise filters
    auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell);

    std::optional<MachinePosXYZ> center = true_top_center;
    for (Phase phase = Phase::first; phase != Phase::_count; phase = Phase(std::to_underlying(phase) + 1)) {
        if (!center.has_value()) {
            return std::nullopt;
        }
        center = get_single_xyz_center(center.value(), tool, phase);
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
inline bool calibrate_toolhead(measurements_t &m, const uint8_t extruder) {
    if (extruder >= PhysicalToolIndex::count) {
        SERIAL_ECHOLNPAIR("G425: Tool ", extruder, " not valid.");
        debug_assert(false);
        return false;
    }
    const auto tool = PhysicalToolIndex::from_raw(extruder);

    TEMPORARY_BACKLASH_CORRECTION(all_on);
    TEMPORARY_BACKLASH_SMOOTHING(0.0f);

    // Disable PA to reduce filter delay during probe analysis
    pressure_advance::PressureAdvanceDisabler pa_disabler;

#if HOTENDS > 1
    set_nozzle(m, extruder);
#else
    // #error dead code found by automatic analyses (see BFW-5461)
    UNUSED(extruder);
#endif

    const std::optional<MachinePosXYZ> center = get_xyz_center(extruder);
    if (!center.has_value()) {
        // TODO:
        SERIAL_ECHOLNPAIR("G425: Tool ", extruder, " center not found.");
        return false;
    }
    m.obj_center = center.value();
    m.pos_error = true_top_center - center.value();

// Adjust the hotend offset
#if HAS_HOTEND_OFFSET
    #if HAS_X_CENTER
    hotend_offset[tool].x += m.pos_error.x;
    #endif
    #if HAS_Y_CENTER
    hotend_offset[tool].y += m.pos_error.y;
    #endif
    hotend_offset[tool].z += m.pos_error.z;
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
    return true;
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

    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        calibrate_toolhead(m, tool.to_raw());
    }

#if HAS_HOTEND_OFFSET
    normalize_hotend_offsets();
    #if HAS_TOOLCHANGER()
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
    // #error dead code found by automatic analyses (see BFW-5461)
    calibrate_backlash(m);
#endif

    tool_change(NoTool {}, tool_return_t::no_return);
}

/**
 * Perform a full auto-calibration routine - simple implementation:
 *
 * This is simplified version that:
 * - un-applies, zeroes all offsets
 * - measures pin centers for all the pins
 * - computes new offsets
 */
inline bool calibrate_all_simple() {
    // Disable E steppers to reduce noise on loadcell
    disable_e_steppers();

#if HAS_CRASH_DETECTION()
    // Disable crash recovery. It would recover, but the measurement will be inaccurate anyway.
    Crash_Temporary_Deactivate ctd;
#endif

    // Reset planner state
    planner.synchronize();
    planner.reset_position();

    // Disable PA to reduce filter delay during probe analysis
    pressure_advance::PressureAdvanceDisabler pa_disabler;

    // Zero hotend offsets
    reset_hotend_offsets();
    hotend_currently_applied_offset = xyz_pos_t {};

    bool failed = false;
    // Measure centers
    StrongIndexArray<MachinePosXYZ, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> centers;
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        tool_change(tool, tool_return_t::no_return);
        std::optional<MachinePosXYZ> center = get_xyz_center(tool.to_raw());
        if (!center.has_value()) {
            SERIAL_ECHOLNPAIR("G425: Tool ", tool.to_raw(), " center not found.");
            failed = true;
            break; // Do not continue with the next tool
        }
        centers[tool] = center.value();
        metric_record_custom(
            &metric_center,
            ",t=%u x=%.3f,y=%.3f,z=%.3f",
            tool.to_raw(),
            static_cast<double>(centers[tool].x),
            static_cast<double>(centers[tool].y),
            static_cast<double>(centers[tool].z));
    }

    if (failed) {
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::filament_change));
        marlin_server::set_warning(WarningType::NozzleDoesNotHaveRoundSection);
        return false;
    }

    // Pick zero offset tool to be sure no offset is applied on toolchange
    tool_change(NoTool {}, tool_return_t::no_return);

    // Apply the offset
    for (auto tool : PhysicalToolIndex::all()) {
        if (!prusa_toolchanger.getTool(tool).is_enabled()) {
            continue;
        }
        // One might ask why the "-" in front of the centers[e].
        // Remember, when tool is bend +x it will find the object at the position -x.
        // hotend_offset is tag-ambiguous, so it needs a bit of a whack to fit
        hotend_offset[tool] = -centers[tool].to_tag<NativePosTag>();
    }
    normalize_hotend_offsets();

    // Check offsets
    for (auto tool : PhysicalToolIndex::all()) {
        if (!tool.is_enabled()) {
            hotend_offset[tool] = {};
            continue;
        }

        if (hotend_offset[tool].x < X_MIN_OFFSET || hotend_offset[tool].x > X_MAX_OFFSET) {
            fatal_error(ErrCode::ERR_MECHANICAL_TOOL_OFFSET_OUT_OF_BOUNDS, tool.to_raw() + 1, 'X', static_cast<double>(hotend_offset[tool].x), static_cast<double>(X_MIN_OFFSET), static_cast<double>(X_MAX_OFFSET));
        }
        if (hotend_offset[tool].y < Y_MIN_OFFSET || hotend_offset[tool].y > Y_MAX_OFFSET) {
            fatal_error(ErrCode::ERR_MECHANICAL_TOOL_OFFSET_OUT_OF_BOUNDS, tool.to_raw() + 1, 'Y', static_cast<double>(hotend_offset[tool].y), static_cast<double>(Y_MIN_OFFSET), static_cast<double>(Y_MAX_OFFSET));
        }
        if (hotend_offset[tool].z < Z_MIN_OFFSET || hotend_offset[tool].z > Z_MAX_OFFSET) {
            fatal_error(ErrCode::ERR_MECHANICAL_TOOL_OFFSET_OUT_OF_BOUNDS, tool.to_raw() + 1, 'Z', static_cast<double>(hotend_offset[tool].z), static_cast<double>(Z_MIN_OFFSET), static_cast<double>(Z_MAX_OFFSET));
        }
    }
    prusa_toolchanger.save_tool_offsets();

    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        osDelay(100); // Some time to send the metric before sending the next one
        metric_record_custom(
            &metric_offset,
            ",t=%u x=%.3f,y=%.3f,z=%.3f",
            tool.to_raw(),
            static_cast<double>(hotend_offset[tool].x),
            static_cast<double>(hotend_offset[tool].y),
            static_cast<double>(hotend_offset[tool].z));
    }
    return true;
}

} // anonymous namespace

bool full_calibration() {
    TEMPORARY_SOFT_ENDSTOP_STATE(false);
    TEMPORARY_BED_LEVELING_STATE(false);

    phase_stepping::EnsureDisabled ps_disabler {};

    if (axis_unhomed_error()) {
        return false;
    }

    return calibrate_all_simple();
}

/**
 * \addtogroup G-Codes
 */

/**
 *### G425: Perform calibration with calibration object <a href="https://reprap.org/wiki/G-code#G425:_Perform_auto-calibration_with_calibration_cube">G425: Perform auto-calibration with calibration cube</a>
 *
 * Only XL
 *
 *#### Usage
 *
 *    G425
 *
 */
void GcodeSuite::G425() {
    full_calibration();
}
/** @}*/
