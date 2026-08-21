/// @file
/// @brief Tool offset calibration (Z-offset via probing + XY-offset with tool_offset board)

#include "tool_offset_calibration.hpp"

#include <array>
#include <algorithm>
#include <bitset>
#include <cmath>
#include <optional>

#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/motion.h>
#include <Marlin/src/module/planner.h>
#include <Marlin/src/module/tool_change.h>
#include <Marlin/src/module/probe.h>
#include <Marlin/src/module/prusa/toolchanger.h>
#include <marlin_server.hpp>
#include <common/printer_model.hpp>
#include <warning_type.hpp>
#include <tool_index.hpp>
#include <tools_mapping.hpp>
#include <gcode/gcode_info.hpp>
#include <bsod.h>
#include <filament.hpp>
#include <config_store/store_instance.hpp>
#include <logging/log.hpp>
#include <metric.h>
#include <random/random.h>
#include <mapi/parking.hpp>
#include <raii/scope_guard.hpp>
#include <feature/print_status_message/print_status_message_guard.hpp>
#include <feature/contactless_offset/contactless_offset.hpp>
#include <feature/pressure_advance/pressure_advance_config.hpp>
#include <utils/variant_utils.hpp>
#include <option/has_indx.h>
#include <option/has_toolchanger.h>
#include <feature/gcode_exception/gcode_exception.hpp>
#include <puppies/tool_offset_sensor.hpp>

#include <option/has_nozzle_cleaner.h>
#if HAS_NOZZLE_CLEANER()
    #include <nozzle_cleaner.hpp>
#endif
#include <option/has_nozzle_cleaner_lite.h>
#if HAS_NOZZLE_CLEANER_LITE()
    #include <nozzle_cleaner_lite.hpp>
#endif
#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

static_assert(HAS_TOOLCHANGER(), "Needs toolchanger");

LOG_COMPONENT_DEF(ToolOffsetCalib, logging::Severity::info);

METRIC_DEF(metric_sensor_pos, "tool_off_sensor_pos", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_sensor_displacement, "tos_displacement", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
// Per-tool hotend offset (result of tool offset calibration) [mm]
METRIC_DEF(metric_tool_offset, "tool_offset", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);

namespace {
constexpr xy_pos_t POS_TOOL_0 = { 25.0f, 5.0f };
constexpr xy_pos_t POS_TOOL_LAST = { 65.0f, 5.0f };

// Safe Z height for travel moves between probes
constexpr float SAFE_Z_HEIGHT = 3.0f;

#if PRINTER_IS_PRUSA_XL()
/// Maximum allowable Z offset difference between tools, in mm
/// If exceeded, the print is not allowed to continue
constexpr float MAX_Z_OFFSET_DIFFERENCE = 4.0f;

/// Maximum allowable XY offset spread (max - min) per axis after normalization, in mm
/// If exceeded, the print is not allowed to continue
constexpr float MAX_XY_OFFSET_DIFFERENCE = 4.0f;

// Fallback temperatures if no filament is loaded
constexpr int16_t DEFAULT_CLEANING_TEMP = 180;
constexpr int16_t DEFAULT_Z_PROBING_TEMP = 150;
constexpr int16_t DEFAULT_XY_PROBING_TEMP = 150;

#elif PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
/// Maximum allowable Z offset difference between tools, in mm
/// If exceeded, the print is not allowed to continue
constexpr float MAX_Z_OFFSET_DIFFERENCE = 0.4f;

/// Maximum allowable XY offset spread (max - min) per axis after normalization, in mm
/// If exceeded, the print is not allowed to continue
constexpr float MAX_XY_OFFSET_DIFFERENCE = 0.4f;

// Fallback temperatures if no filament is loaded
constexpr int16_t DEFAULT_CLEANING_TEMP = 220;
constexpr int16_t DEFAULT_Z_PROBING_TEMP = 170;
constexpr int16_t DEFAULT_XY_PROBING_TEMP = 170;
#else
    #error "No tool offset calibration config for this printer"
#endif

struct ToolTemperatures {
    int16_t cleaning; // nozzle temp for purge/clean
    int16_t z_probing; // cooled-down temp for Z probing
    int16_t xy_probing; // cooled-down temp for XY probing
};

class AxisAlignedBoundingBox {
public:
    void add(const xyz_pos_t &point) {
        min_.x = std::min(min_.x, point.x);
        min_.y = std::min(min_.y, point.y);
        min_.z = std::min(min_.z, point.z);
        max_.x = std::max(max_.x, point.x);
        max_.y = std::max(max_.y, point.y);
        max_.z = std::max(max_.z, point.z);
    }

    const xyz_pos_t &min() const { return min_; }
    const xyz_pos_t &max() const { return max_; }
    xyz_pos_t size() const { return max_ - min_; }

private:
    static constexpr float hi = std::numeric_limits<float>::max();
    static constexpr float lo = std::numeric_limits<float>::lowest();
    xyz_pos_t min_ { hi, hi, hi };
    xyz_pos_t max_ { lo, lo, lo };
};

/// Get nozzle temperatures for a physical tool from its loaded filament.
/// Uses tool mapping to find the gcode tool, then looks up the filament type.
/// xy_probing temp is set to a default since it does not depend on used filament or tool
ToolTemperatures get_tool_temperatures(PhysicalToolIndex physical_tool) {
    const auto virtual_tool = stdext::get_optional<VirtualToolIndex>(physical_tool.currently_selected_virtual_tool());
    const FilamentType filament = virtual_tool.has_value() ? FilamentType::for_tool_heuristic(*virtual_tool) : FilamentType::none;

    if (filament != FilamentType::none) {
        const auto params = filament.parameters();
#if HAS_NOZZLE_CLEANER()
        return { params.nozzle_temperature, params.nozzle_preheat_temperature, DEFAULT_XY_PROBING_TEMP };
#elif HAS_NOZZLE_CLEANER_LITE()
        // The lite cleaner cleans at the preheat temperature and rests on the
        // touchpoint until the cool-down temperature; Z probing then runs at
        // that temperature (as hot as possible without ooze, so the nozzle
        // thermal expansion stays close to printing conditions).
        const int16_t cleaning_temp = params.nozzle_preheat_temperature;
        return { cleaning_temp, static_cast<int16_t>(cleaning_temp - nozzle_cleaner_lite::cooldown_temp_diff), DEFAULT_XY_PROBING_TEMP };
#else
    #error "No nozzle cleaner available, no cleaning temperature defined for this printer"
#endif
    } else {
        return { DEFAULT_CLEANING_TEMP, DEFAULT_Z_PROBING_TEMP, DEFAULT_XY_PROBING_TEMP };
    }
}

/// Set the hotend target and wait for it. When heating, return as soon as the temperature
/// crosses the target, skipping the residency settle (M109 C-style) to save time.
/// wait_temp triggers on current >= temp, so it must not be passed on a cool-down wait.
void set_temp_and_wait_reached(PhysicalToolIndex tool, int16_t temp, bool fan_cooling = false) {
    thermalManager.setTargetHotend(temp, tool);
    const bool heating = thermalManager.degHotend(tool) < temp;
    thermalManager.wait_for_hotend(tool, {
                                             .no_wait_for_cooling = false,
                                             .fan_cooling = fan_cooling,
                                             .early_return_temperature = heating ? std::optional<float>(temp) : std::nullopt,
                                         });
}

/// Return a random float in [-r_param, +r_param]
float random_jitter(uint8_t r_param) {
    const float normalized = rand_f_from_u(rand_u()); // [0.0, 1.0]
    return (normalized * 2.0f - 1.0f) * r_param; // [-r_param, +r_param]
}

/// Probe Z at a given XY position, averaging multiple measurements.
/// Returns NaN on failure.
/// Asks for no tool corrections: the hotend offset and the nozzle length are what this
/// calibration is measuring, so folding them in would make it chase its own tail.
float probe_z_at(const xy_pos_t &pos, uint8_t probe_count) {
    return probe_at_point(pos, PROBE_PT_NONE, 1, true, probe_count, ApplyToolCorrections::no);
}

/// Park at nozzle cleaner and run the cleaning sequence.
/// In `Calibration` context the nozzle cleaner is not yet calibrated (chicken-and-egg) so the
/// auto-clean sequence is skipped; the caller has already prompted the user to clean manually.
/// @return true if cleaning succeeded, false on failure or abort
bool prepare_tool(PhysicalToolIndex tool, [[maybe_unused]] tool_offset_calibration::Context context) {
    // Guard against a silently failed toolchange — do not drive to a parked tool's brush
    const std::optional<PhysicalToolIndex> selected = PhysicalToolIndex::currently_selected_opt();
    if (!selected.has_value() || selected.value() != tool) {
        log_error(ToolOffsetCalib, "Tool %u is not the selected tool, cannot clean", tool.to_raw());
        return false;
    }
#if !HAS_INDX()
    // Physical pick detection is only available on the dwarf toolchanger
    if (!prusa_toolchanger.getTool(tool).is_picked()) {
        log_error(ToolOffsetCalib, "Tool %u is not physically picked, cannot clean", tool.to_raw());
        return false;
    }
#endif

    const ToolTemperatures temps = get_tool_temperatures(tool);

#if HAS_NOZZLE_CLEANER()
    if (context == tool_offset_calibration::Context::Print) {
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::nozzle_cleaner_approach));

        if (!nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::eject_blob)) {
            return false;
        }

        // Purge and clean at cleaning temperature
        set_temp_and_wait_reached(tool, temps.cleaning);
        if (!nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::purge_clean)) {
            return false;
        }

        // Cool down to probing temperature with print fan on to speed it up
        thermalManager.setTargetHotend(temps.z_probing, tool);
        const uint8_t saved_fan_speed = thermalManager.get_print_fan_speed();
        thermalManager.set_print_fan_speed(255);
        ScopeGuard restore_fan([&] {
            thermalManager.set_print_fan_speed(saved_fan_speed);
        });

        // Deep clean at probing temperature
        thermalManager.wait_for_hotend(tool, { .no_wait_for_cooling = false });
        if (!nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::deep_clean)) {
            return false;
        }

        // park out of cleaner area
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::nozzle_cleaner_exit));
    } else {
        // Calibration context: nozzle was cleaned by the user and Z is not probed here. Heat
        // straight to the XY-probing temperature (lower than the Z-probing temperature, so no
        // cool-down needed afterwards).
        set_temp_and_wait_reached(tool, temps.xy_probing);
    }
#elif HAS_NOZZLE_CLEANER_LITE()
    // The lite cleaner self-locates (homes and probes its own Z reference) and handles
    // nozzle heating internally, so it can run in both contexts. As probing_tool it
    // rests on the touchpoint until the cool-down temperature and keeps that target
    // for the Z probing that follows.
    if (nozzle_cleaner_lite::is_available()) {
        if (!nozzle_cleaner_lite::clean(nozzle_cleaner_lite::CleanType::probing_tool)) {
            return false;
        }
    }

    // Already reached after a successful clean; establishes the Z-probing
    // temperature when the cleaner is not installed.
    set_temp_and_wait_reached(tool, temps.z_probing);
#else
    #error "Not defined behavior for this printer configuration"
#endif
    return true;
}

/// Park the head with low Z at the front so the user can inspect/clean the nozzle
/// and prompt with Retry/Abort.
/// Returns: Response::Retry on a successful retry path, Response::Abort otherwise.
Response prompt_retry(WarningType warning, tool_offset_calibration::Context context) {
    // If the user already aborted, skip the dialog and the parking move. This covers a stopped
    // print as well as a calibration-wizard Abort, which quick-stops the planner (-> draining()).
    if (marlin_server::aborting_or_aborted() || planner.draining()) {
        return Response::Abort;
    }

    constexpr float park_clean_z = 100.0f;
    const mapi::ParkingPosition park_cleaning_position = mapi::ParkingPosition::from_xyz_pos({ { { (X_MIN_POS + X_MAX_POS) / 2.0f, 10.0f, park_clean_z } } });
    mapi::park(park_cleaning_position);

    if (marlin_server::prompt_warning(warning) != Response::Retry) {
        if (context == tool_offset_calibration::Context::Print) {
            marlin_server::print_abort();
        }
        return Response::Abort;
    }

    return Response::Retry;
}

/// Park the head with low Z at the front so the user can inspect/clean the nozzle
/// and prompt with Retry/Abort.
/// On retry performs nozzle cleaning
/// Returns: Response::Retry on a successful retry path, Response::Abort otherwise.
Response prompt_retry_with_cleaning(PhysicalToolIndex tool, WarningType warning, tool_offset_calibration::Context context) {
    auto response = prompt_retry(warning, context);

    while (response == Response::Retry && !prepare_tool(tool, context)) {
        log_error(ToolOffsetCalib, "Nozzle cleaning failed");
        response = prompt_retry(warning, context);
    }
    return response;
}

using PhysicalToolSet = std::bitset<PhysicalToolIndex::count>;

/// Collect the unique set of physical tools needed for the current print,
/// based on tool mapping and spool join configuration.
PhysicalToolSet collect_used_physical_tools() {
    PhysicalToolSet seen;

    // Walk each gcode tool that is used in the print
    auto &gcode_info = GCodeInfo::getInstance();
    for (auto gcode_tool : GcodeToolIndex::all()) {
        if (!gcode_info.get_extruder_info(gcode_tool).used()) {
            continue;
        }

        // Map gcode → virtual (respects tool mapper)
        auto virtual_tool = stdext::get_optional<VirtualToolIndex>(gcode_tool.to_virtual());
        while (virtual_tool) {
            seen.set(virtual_tool->to_physical().to_raw());

#if HAS_SPOOL_JOIN()
            virtual_tool = spool_join.get_spool_2(*virtual_tool);
#else
            virtual_tool = std::nullopt;
#endif
        }
    }

    return seen;
}

/// Collect all physically enabled tools.
PhysicalToolSet collect_all_enabled_tools() {
    PhysicalToolSet seen;
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        seen.set(tool.to_raw());
    }
    return seen;
}

/// Reset Z tool offsets to zero (runtime + EEPROM) to avoid corruption from previous prints
void reset_z_tool_offsets() {
    for (auto tool : PhysicalToolIndex::all()) {
        hotend_offset[tool].z = 0;
    }
    hotend_currently_applied_offset.z = 0;
    prusa_toolchanger.save_tool_offsets();
}

} // namespace

namespace tool_offset_calibration {

bool calibrate_xy_offset(PhysicalToolIndex tool, const tool_offset::ProbingConfig &config, Context context) {
    if (!is_hardware_available()) {
        bsod("Tool offset sensor requested, but not available on this printer");
    }
    const auto selected_tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (!selected_tool.has_value()) {
        log_error(ToolOffsetCalib, "failed: no tool selected");
        return false;
    }
    if (tool != selected_tool.value()) {
        log_error(ToolOffsetCalib, "Selected tool must be picked");
        return false;
    }

    // Reset planner state
    planner.synchronize();
    planner.reset_position();

    // Disable PA to reduce filter delay during probe analysis
    pressure_advance::PressureAdvanceDisabler pa_disabler;

    // Perform the measurement for picked tool. The orchestrator creates the
    // sensor(s) it needs via the factory (one CH1 for single-coil, CH0+CH1 for
    // the two XLS coils).
    tool_offset::ToolOffset current_ho = {
        .x = hotend_offset[tool].x,
        .y = hotend_offset[tool].y,
        .z = hotend_offset[tool].z,
    };

    auto offset_for_measurement = current_ho;
    offset_for_measurement.x = std::clamp(offset_for_measurement.x, static_cast<float>(X_MIN_OFFSET), static_cast<float>(X_MAX_OFFSET));
    offset_for_measurement.y = std::clamp(offset_for_measurement.y, static_cast<float>(Y_MIN_OFFSET), static_cast<float>(Y_MAX_OFFSET));
    // Zero hotend offset and currently applied offset
    reset_hotend_offset(tool);
    hotend_currently_applied_offset = xyz_pos_t {};

    while (true) {
        // wait for calibration temperature (if probing is higher than upper limit)
        // Restore temp is one level above in run()
        const ToolTemperatures temps = get_tool_temperatures(tool);
        set_temp_and_wait_reached(tool, temps.xy_probing, /*fan_cooling=*/true);

        log_info(ToolOffsetCalib, "XY offset measurement");
        auto result = tool_offset::measure_current_tool_offset(config, offset_for_measurement);
        if (result.has_value()) {
            log_info(ToolOffsetCalib, "Measured XY offset: X=%.3f Y=%.3f", static_cast<double>(result->x), static_cast<double>(result->y));
            // Store newly measured offsets only for XY, keep actual for Z
            hotend_offset[tool].x = result->x;
            hotend_offset[tool].y = result->y;
            hotend_offset[tool].z = current_ho.z;
            prusa_toolchanger.save_tool_offset(tool);
            hotend_currently_applied_offset = hotend_offset[tool];
            return true;
        } else {
            log_error(ToolOffsetCalib, "Measurement failed: %s", result.error());
        }

        if (prompt_retry_with_cleaning(tool, WarningType::ToolOffsetCalibrationFailed, context) != Response::Retry) {
            // Restore the original offsets
            hotend_offset[tool].x = current_ho.x;
            hotend_offset[tool].y = current_ho.y;
            hotend_offset[tool].z = current_ho.z;
            prusa_toolchanger.save_tool_offset(tool);
            hotend_currently_applied_offset = xyz_pos_t { current_ho.x, current_ho.y, current_ho.z };
            log_error(ToolOffsetCalib, "User aborted XY offset calibration for tool %u", tool.to_raw());
            return false;
        }
        log_info(ToolOffsetCalib, "User requested XY offset retry for tool %u", tool.to_raw());

#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
        // Re-park over the sensor before re-entering the loop. Single-coil only:
        // the dual-coil flow has a coil per axis and repositions itself per coil
        // at measurement entry.
        do_blocking_move_to_z(config.coil_x.position.z + config.safe_z_height);
        do_blocking_move_to_xy(config.coil_x.position.x, config.coil_x.position.y);
#endif
    }
}

#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
void apply_stored_sensor_position(tool_offset::ProbingConfig &config) {
    const xy_pos_t configured { config.coil_x.position.x, config.coil_x.position.y };
    const auto stored = config_store().tool_offset_sensor_position.get();
    if (std::abs(stored.x - configured.x) > config.sensor_position_error_threshold || std::abs(stored.y - configured.y) > config.sensor_position_error_threshold) {
        log_error(ToolOffsetCalib, "Stored sensor position (X=%.1f Y=%.1f) is too far from default (X=%.1f Y=%.1f), fallback to default values",
            static_cast<double>(stored.x),
            static_cast<double>(stored.y),
            static_cast<double>(configured.x),
            static_cast<double>(configured.y));
        config_store().tool_offset_sensor_position.set(configured);
        metric_record_custom(&metric_sensor_pos, " x=%.3f,y=%.3f",
            static_cast<double>(configured.x),
            static_cast<double>(configured.y));
        return;
    }
    // Stored position is more accurate than the default
    tool_offset::set_single_coil_position(config, stored);
}
#else
xy_pos_t apply_stored_sensor_displacement(tool_offset::ProbingConfig &config) {
    const xy_pos_t stored = config_store().tool_offset_sensor_displacement.get();
    if (std::abs(stored.x) > config.sensor_displacement_error_threshold || std::abs(stored.y) > config.sensor_displacement_error_threshold) {
        log_error(ToolOffsetCalib, "Stored sensor displacement (X=%.1f Y=%.1f) too large, resetting to zero",
            static_cast<double>(stored.x),
            static_cast<double>(stored.y));
        const xy_pos_t zero { { { 0.f, 0.f } } };
        config_store().tool_offset_sensor_displacement.set(zero);
        metric_record_custom(&metric_sensor_displacement, " x=%.3f,y=%.3f",
            static_cast<double>(zero.x),
            static_cast<double>(zero.y));
        return zero;
    }

    // Compute the tightest per-axis displacement bounds so all shifted geometry stays in travel.
    const float half_x = config.coil_x.sensing_distance / 2.f;
    const float half_y = config.coil_y.sensing_distance / 2.f;
    const float dx_lo = std::max({ static_cast<float>(X_MIN_POS) - config.coil_x.position.x + half_x,
        static_cast<float>(X_MIN_POS) - config.coil_y.position.x,
        static_cast<float>(X_MIN_POS) - config.z_probe_position.x });
    const float dx_hi = std::min({ static_cast<float>(X_MAX_POS) - config.coil_x.position.x - half_x,
        static_cast<float>(X_MAX_POS) - config.coil_y.position.x,
        static_cast<float>(X_MAX_POS) - config.z_probe_position.x });
    const float dy_lo = std::max({ static_cast<float>(Y_MIN_POS) - config.coil_y.position.y + half_y,
        static_cast<float>(Y_MIN_POS) - config.coil_x.position.y,
        static_cast<float>(Y_MIN_POS) - config.z_probe_position.y });
    const float dy_hi = std::min({ static_cast<float>(Y_MAX_POS) - config.coil_y.position.y - half_y,
        static_cast<float>(Y_MAX_POS) - config.coil_x.position.y,
        static_cast<float>(Y_MAX_POS) - config.z_probe_position.y });

    const xy_pos_t displacement { { { std::clamp(stored.x, dx_lo, dx_hi), std::clamp(stored.y, dy_lo, dy_hi) } } };
    if (displacement.x != stored.x || displacement.y != stored.y) {
        log_warning(ToolOffsetCalib, "Sensor displacement clamped from (%.3f, %.3f) to (%.3f, %.3f) to stay within travel",
            static_cast<double>(stored.x), static_cast<double>(stored.y),
            static_cast<double>(displacement.x), static_cast<double>(displacement.y));
    }

    config.coil_x.position.x += displacement.x;
    config.coil_x.position.y += displacement.y;
    config.coil_y.position.x += displacement.x;
    config.coil_y.position.y += displacement.y;
    config.z_probe_position.x += displacement.x;
    config.z_probe_position.y += displacement.y;

    return displacement;
}
#endif

bool run(uint8_t r_param, uint8_t probe_count, Context context, const ProgressCallback &progress_cb) {
    if (!is_hardware_available()) {
        bsod("Tool offset sensor requested, but not available on this printer");
    }

    // PrintStatusMessageGuard is only meaningful while a print is running; in Calibration context
    // a wizard FSM drives the UI, so leave the status bar alone.
    std::optional<PrintStatusMessageGuard> status_guard;
    if (context == Context::Print) {
        status_guard.emplace();
    }
    reset_z_tool_offsets(); // Clear old Z offsets to avoid interference with calibration

    log_info(ToolOffsetCalib, "Starting tool offset calibration");

    // Restore the originally picked tool on any exit path
    const auto original_tool = PhysicalToolIndex::currently_selected();
    ScopeGuard restore_tool([&] {
        if (!tool_change(stdext::to_variant(original_tool), tool_return_t::no_return)) {
            log_error(ToolOffsetCalib, "Failed to restore original tool");
        }

        // Do not allow RAM-EEPROM mismatch of tool offsets, save whatever is currently set, even if we fail
        prusa_toolchanger.save_tool_offsets();
    });

    // Get the nozzle clear of the bed/sensor on any failure exit
    // Noop while unwinding and appending 5mm lift when print is being aborted
    ScopeGuard raise_z_on_failure([&] {
        mapi::park({ .z = mapi::ParkingPosition::Relative { FAILURE_Z_RAISE } });
    });

    if (!GcodeSuite::G28_no_parser(true, true, true, G28Flags { .only_if_needed = true })) {
        log_error(ToolOffsetCalib, "Homing failed");
        return false;
    }

    // In Print context, calibrate only the tools the print needs to save time.
    // GCodeInfo retains the last print's data, so it must not be consulted
    // outside of Context::Print.
    PhysicalToolSet used_physical_tools;
    if (context == Context::Print) {
        used_physical_tools = collect_used_physical_tools();
    }
    if (used_physical_tools.none()) {
        used_physical_tools = collect_all_enabled_tools();
    }

    const auto num_tools = static_cast<uint8_t>(used_physical_tools.count());
    if (num_tools == 0) {
        log_error(ToolOffsetCalib, "No tools found");
        return false;
    }

    log_info(ToolOffsetCalib, "Calibrating %u tool(s)", num_tools);

#if HAS_INDX()
    // A single-physical-tool print can't change tools: pin the last-picked tool to it so a
    // mid-print reset recovers it (via the nozzle sensor) without writing EEPROM during the print.
    if (context == Context::Print) {
        std::optional<PhysicalToolIndex> single_tool;
        if (num_tools == 1) {
            auto mask = used_physical_tools.to_ullong();
            single_tool = PhysicalToolIndex::from_raw(std::countr_zero(mask));
        }
        prusa_toolchanger.set_single_print_tool(single_tool);
    }
#endif

#if HAS_INDX()
    // For INDX Z probing is only done from a running print — selftest skips it to save time.
    // Z offsets are then left at zero until the next G427 call from a print start.
    const bool measure_z = (context == Context::Print);
#elif PRINTER_IS_PRUSA_XL()
    // XLS does not perform tool offset calibration during print, so for XLS measure Z
    // during selftest/calibration on XLS - the only place where the Z offset are calculated
    const bool measure_z = true;
#else
    #error "Not defined behavior for this printer configuration"
#endif

#if HAS_NOZZLE_CLEANER_LITE() && PRINTER_IS_PRUSA_XL()
    // Start heating all used tools to the cleaning temperature in parallel to save time;
    std::array<int16_t, PhysicalToolIndex::count> saved_nozzle_targets {};
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        if (!used_physical_tools.test(tool.to_raw())) {
            continue;
        }
        saved_nozzle_targets[tool.to_raw()] = thermalManager.degTargetHotend(tool);
        thermalManager.setTargetHotend(get_tool_temperatures(tool).cleaning, tool);
    }
    ScopeGuard restore_nozzle_targets([&] {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            if (used_physical_tools.test(tool.to_raw())) {
                thermalManager.setTargetHotend(saved_nozzle_targets[tool.to_raw()], tool);
            }
        }
    });
#endif

    struct ProbeResult {
        float z;
        xy_pos_t pos;
    };

    // Each pass runs the per-tool measurement loop followed by the deviation
    // sanity checks. If any deviation check fails and the user chooses Retry,
    // prompt_retry allows to re-clean the nozzle and we restart the whole
    // pass with offsets reset.
    while (true) {
        // Re-derive the probing config from defaults each pass: a previous pass's
        // drift correction may have updated the stored values, and the dual-coil
        // displacement application shifts the config additively.
        auto probing_config = tool_offset::get_default_probing_config();
#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
        // The stored sensor position tracks a single coil; dual-coil (XLS) would
        // need two stored positions (TODO WP5.4: per-coil storage + migration).
        apply_stored_sensor_position(probing_config);
#else
        const xy_pos_t applied_sensor_displacement = apply_stored_sensor_displacement(probing_config);
#endif
        reset_hotend_offsets();
        hotend_currently_applied_offset = xyz_pos_t {};

        // Helper: probe Z at the given mapped index position (with jitter)
        auto probe_at = [&](PhysicalToolIndex tool, uint8_t probe_index) -> std::optional<ProbeResult> {
            // total without -1 is intentional - the last spot (probe_index == num_tools) is reserved for second reference measurement
            const float t = static_cast<float>(probe_index) / static_cast<float>(num_tools);
            const xy_pos_t pos {
                std::clamp(POS_TOOL_0.x + (POS_TOOL_LAST.x - POS_TOOL_0.x) * t + random_jitter(r_param), static_cast<float>(X_MIN_POS), static_cast<float>(X_MAX_POS)),
                std::clamp(POS_TOOL_0.y + (POS_TOOL_LAST.y - POS_TOOL_0.y) * t + random_jitter(r_param), static_cast<float>(Y_MIN_POS), static_cast<float>(Y_MAX_POS)),
            };

            log_info(ToolOffsetCalib, "Probe count: %d", probe_count);
            const float z = probe_z_at(pos, probe_count);
            if (std::isnan(z)) {
                return std::nullopt;
            }
            log_info(ToolOffsetCalib, "Tool %u Z=%.3f at (%.1f, %.1f)", tool.to_raw(), static_cast<double>(z), static_cast<double>(pos.x), static_cast<double>(pos.y));
            do_blocking_move_to_z(SAFE_Z_HEIGHT);
            return ProbeResult { z, pos };
        };

        bool need_pass_retry = false;
        // Helper: Prompt the user to Retry/Abort if any check fails. Returns true if the caller should retry the whole pass, false to continue.
        auto handle_tool_failure = [&]() -> bool {
            if (prompt_retry(WarningType::ToolOffsetCalibrationFailed, context) == Response::Retry) {
                need_pass_retry = true;
                return true;
            }
            return false;
        };

        // Issue print_abort only if running from a print. In a calibration wizard the
        // caller handles wizard teardown itself.
        auto abort_print_if_needed = [&] {
            if (context == Context::Print) {
                marlin_server::print_abort();
            }
        };

        ProbeResult ref_first;
        ProbeResult ref_last;

        AxisAlignedBoundingBox aabb;
        xyz_pos_t sum_offsets {};
        uint8_t no_summed_offsets = 0;

        uint8_t step = 0;
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            if (!used_physical_tools.test(tool.to_raw())) {
                continue;
            }

            if (status_guard) {
                status_guard->update<PrintStatusMessage::Type::tool_offset_calibrating>({
                    .progress = { .current = static_cast<float>(step + 1), .target = static_cast<float>(num_tools) },
                    .tool = tool.to_raw(),
                });
            }
            if (progress_cb) {
                if (!progress_cb({
                        .step = static_cast<uint8_t>(step + 1),
                        .total_steps = num_tools,
                        .tool = tool,
                    })) {
                    log_info(ToolOffsetCalib, "Tool offset calibration aborted by progress callback");
                    return false;
                }
            }

            if (!tool_change(stdext::to_variant(tool), tool_return_t::no_return)) {
                log_error(ToolOffsetCalib, "Tool change to tool %u failed", tool.to_raw());
                return false;
            }

#if HAS_NOZZLE_CLEANER_LITE() && PRINTER_IS_PRUSA_XL()
            // The pre-heat above already overwrote the target, so restore the genuinely
            // previous temperature captured before it. This also lets a finished tool cool
            // down while the remaining tools are being calibrated.
            const int16_t saved_temp = saved_nozzle_targets[tool.to_raw()];
#else
            const int16_t saved_temp = thermalManager.degTargetHotend(tool);
#endif
            ScopeGuard restore_temp([&] {
                thermalManager.setTargetHotend(saved_temp, tool);
            });

            if (!prepare_tool(tool, context)) {
                log_error(ToolOffsetCalib, "Tool %u cleaning failed (step %u)", tool.to_raw(), step);
                if (handle_tool_failure()) {
                    break;
                }
                abort_print_if_needed();
                return false;
            }

            // The Abort button quick-stops the planner (sets draining()), which is what broke the
            // heat-up wait above. Exit now - the probe/scan below can't run while draining anyway.
            if (gcode_exceptions().is_unwinding()) {
                return false;
            }

            if (num_tools == 1 && measure_z) {
                // With one-tool print, we still need to do prepare_tool which runs the nozzle cleaner
                // But we don't need to do any offset calibrations, so we can just quit now
                // We don't need to be that precise for the nozzle cleaner
                break;
            }

            if (measure_z) {
                const auto result_opt = probe_at(tool, step);
                if (!result_opt) {
                    log_error(ToolOffsetCalib, "Z probe failed for tool %u (step %u)", tool.to_raw(), step);
                    if (handle_tool_failure()) {
                        break;
                    }
                    abort_print_if_needed();
                    return false;
                }
                const auto result = *result_opt;

                if (step == 0) {
                    // Keep offset at zero as set by the reset at the beginning of the function

                    // If we're measuring more than one tool, we will be measuring all tools on a single X line.
                    // Use the first tool to probe both start and end of the line to interpolate from
                    // This is basically a ghetto MBL
                    ref_first = result;

                    // Probe at last position (with the same first tool)
                    const auto ref_last_opt = probe_at(tool, num_tools);
                    if (!ref_last_opt) {
                        log_error(ToolOffsetCalib, "Z probe failed at reference-line end for tool %u", tool.to_raw());
                        if (handle_tool_failure()) {
                            break;
                        }
                        abort_print_if_needed();
                        return false;
                    }
                    ref_last = *ref_last_opt;

                    log_info(ToolOffsetCalib, "Reference line: Z_first=%.3f at (%.1f,%.1f) Z_last=%.3f at (%.1f,%.1f)",
                        static_cast<double>(ref_first.z), static_cast<double>(ref_first.pos.x), static_cast<double>(ref_first.pos.y),
                        static_cast<double>(ref_last.z), static_cast<double>(ref_last.pos.x), static_cast<double>(ref_last.pos.y));

                } else {
                    // Interpolate expected Z on the reference line at the actual probed X
                    const float t = result.pos.x - ref_first.pos.x;
                    const float z_expected = ref_first.z + (ref_last.z - ref_first.z) * (t / (ref_last.pos.x - ref_first.pos.x));
                    const float z_offset = z_expected - result.z; // Inverted, because +Z is down
                    hotend_offset[tool].z = z_offset;
                    log_info(ToolOffsetCalib, "Tool %u Z offset=%.3f (measured=%.3f expected=%.3f)", tool.to_raw(), static_cast<double>(z_offset), static_cast<double>(result.z), static_cast<double>(z_expected));
                }
            }

            // Note: If we would first calibrate XY offset, it should then give us more precise interpolation on the ghetto MBL line
            // but there is a trade off with filament oozing during calibration
            if (!calibrate_xy_offset(tool, probing_config, context)) {
                // calibrate_xy_offset has already prompted the user and aborted the print on its own
                // path; just return false here. Its inner retry loop handles successful retries.
                return false;
            }

            aabb.add(hotend_offset[tool]);
            sum_offsets += hotend_offset[tool];
            no_summed_offsets++;

            step++;
        }

        if (need_pass_retry) {
            log_info(ToolOffsetCalib, "Restarting tool offset calibration pass (user requested retry)");
            continue;
        }

        // Normalize XY offsets so the average offset is zero.
        // The ScopeGuard at function exit persists hotend_offset to EEPROM.
        const xyz_pos_t spread = aabb.size();
        if (num_tools > 1) {
            if (spread.x > MAX_XY_OFFSET_DIFFERENCE || spread.y > MAX_XY_OFFSET_DIFFERENCE) {
                log_error(ToolOffsetCalib, "XY offset spread too large: X=%.3f Y=%.3f (limit %.3f)",
                    static_cast<double>(spread.x), static_cast<double>(spread.y), static_cast<double>(MAX_XY_OFFSET_DIFFERENCE));
                if (prompt_retry(WarningType::HotendOffsetUnsafeXyDeviation, context) == Response::Retry) {
                    continue;
                }
                return false;
            }

#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
            // Sensor-position drift correction is single-coil only: it rewrites the single
            // stored sensor position and corrects all tools by the midpoint offset. Dual-coil
            // (XLS) measures X and Y over two separate coils, so a single correction is
            // geometrically wrong; dual-coil offsets are stored as measured.
            // TODO WP5.4: per-coil stored positions + the config-store migration.
            const xyz_pos_t average_offset = sum_offsets / no_summed_offsets;
            if (std::abs(average_offset.x) > probing_config.sensor_position_error_threshold || std::abs(average_offset.y) > probing_config.sensor_position_error_threshold) {
                if (prompt_retry(WarningType::HotendOffsetUnsafeSensorXY, context) == Response::Retry) {
                    continue;
                }
                return false;
            } else if (std::abs(average_offset.x) > probing_config.sensor_position_update_threshold || std::abs(average_offset.y) > probing_config.sensor_position_update_threshold) {
                // Detected sensor movement - update stored sensor position and apply correction to all tools.
                // The coil position is where the scan was conducted (either the stored value or the
                // default, depending on what apply_stored_sensor_position resolved to).
                const xyz_pos_t &scanned_at = probing_config.coil_x.position;
                const MachinePosXY new_sensor_position { (scanned_at - average_offset).xy() };
                log_info(ToolOffsetCalib, "Updating stored sensor position: (%.3f, %.3f) -> (%.3f, %.3f)",
                    static_cast<double>(scanned_at.x), static_cast<double>(scanned_at.y),
                    static_cast<double>(new_sensor_position.x), static_cast<double>(new_sensor_position.y));
                config_store().tool_offset_sensor_position.set(new_sensor_position);
                metric_record_custom(&metric_sensor_pos, " x=%.3f,y=%.3f",
                    static_cast<double>(new_sensor_position.x),
                    static_cast<double>(new_sensor_position.y));

                for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
                    // Note: we don't update unused tools, these are reset to zero at the beggining of the function
                    // it is eaiser to recognize then that they were not measured
                    if (!used_physical_tools.test(tool.to_raw())) {
                        continue;
                    }

                    hotend_offset[tool] -= average_offset.xy();
                    metric_record_custom(&metric_tool_offset, ",tool=%u x=%.3f,y=%.3f,z=%.3f",
                        tool.to_raw(),
                        static_cast<double>(hotend_offset[tool].x),
                        static_cast<double>(hotend_offset[tool].y),
                        static_cast<double>(hotend_offset[tool].z));
                }
            }
#else
            // Dual-coil (XLS): the midpoint of the measured offsets encodes the
            // whole-sensor displacement (offset ~ e - (d_true - d_applied); the
            // tool errors e average out, so d_true = d_applied - avg). Once found
            // by the search FSM, the displacement is persisted and applied to the
            // geometry on subsequent passes, so the hunt normally runs at most
            // once per machine. The update threshold is shared with the
            // single-coil path.
            const xyz_pos_t average_offset = sum_offsets / no_summed_offsets;
            if (std::abs(average_offset.x) > probing_config.sensor_displacement_error_threshold || std::abs(average_offset.y) > probing_config.sensor_displacement_error_threshold) {
                // in actual configuration this code is dead
                // TODO: reanalyze the need for this once calibration sequence is finalized for BFW-8420
                if (prompt_retry(WarningType::HotendOffsetUnsafeSensorXY, context) == Response::Retry) {
                    continue;
                }
                return false;
            } else if (std::abs(average_offset.x) > probing_config.sensor_position_update_threshold || std::abs(average_offset.y) > probing_config.sensor_position_update_threshold) {
                const xy_pos_t new_displacement { { { applied_sensor_displacement.x - average_offset.x, applied_sensor_displacement.y - average_offset.y } } };
                log_info(ToolOffsetCalib, "Updating stored sensor displacement: (%.3f, %.3f) -> (%.3f, %.3f)",
                    static_cast<double>(applied_sensor_displacement.x), static_cast<double>(applied_sensor_displacement.y),
                    static_cast<double>(new_displacement.x), static_cast<double>(new_displacement.y));
                config_store().tool_offset_sensor_displacement.set(new_displacement);
                metric_record_custom(&metric_sensor_displacement, " x=%.3f,y=%.3f",
                    static_cast<double>(new_displacement.x),
                    static_cast<double>(new_displacement.y));

                // Correct all measured tools by the midpoint, as in the single-coil path.
                for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
                    if (!used_physical_tools.test(tool.to_raw())) {
                        continue;
                    }
                    hotend_offset[tool] -= average_offset.xy();
                }
            }

            for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
                // Record metrics for all tools
                metric_record_custom(&metric_tool_offset, ",tool=%u x=%.3f,y=%.3f,z=%.3f",
                    tool.to_raw(),
                    static_cast<double>(hotend_offset[tool].x),
                    static_cast<double>(hotend_offset[tool].y),
                    static_cast<double>(hotend_offset[tool].z));
            }
#endif
        }

        if (measure_z && spread.z > MAX_Z_OFFSET_DIFFERENCE) {
            if (prompt_retry(WarningType::HotendOffsetUnsafeZDeviation, context) == Response::Retry) {
                continue;
            }
            return false;
        }

        log_info(ToolOffsetCalib, "Tool offset calibration done");
        raise_z_on_failure.disarm();
        return true;
    }
}

[[nodiscard]] bool is_hardware_available() {
    return buddy::puppies::tool_offset_sensor.is_enabled();
}

} // namespace tool_offset_calibration
