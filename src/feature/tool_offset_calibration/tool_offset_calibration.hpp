/// @file
/// @brief Tool offset calibration (Z-offset via probing + XY-offset with tool_offset board)
#pragma once

#include <functional>

#include <feature/contactless_offset/contactless_offset.hpp>
#include <tool_index.hpp>
#include <option/tool_offset_sensor_geometry.h>

namespace tool_offset_calibration {

/// Relative Z raise to get the nozzle clear of the bed/sensor after a failure or abort
inline constexpr float FAILURE_Z_RAISE = 5.0f;

/// Context in which the calibration is being run.
/// In `Calibration` mode (e.g. selftest / cleaner-calibration wizard) the caller has prompted the
/// user to clean nozzles manually and there is no active print to abort on failure.
enum class Context {
    Print, ///< Invoked from a running print (G427). Auto-cleans via nozzle cleaner; aborts print on failure. Calibrates only the tools the print uses.
    Calibration, ///< Invoked from a calibration wizard. Skips auto-clean; failure just returns false. Calibrates all enabled tools.
};

/// Snapshot of where the calibration is in its per-tool loop.
/// Passed to ProgressCallback once per tool, at the start of that tool's iteration.
struct ProgressReport {
    uint8_t step; ///< 1-based step counter (1..total_steps)
    uint8_t total_steps; ///< number of tools being calibrated in this run
    PhysicalToolIndex tool; ///< physical tool currently being calibrated
};

/// Optional callback invoked once per tool, before that tool's prepare/probe/XY-scan. The
/// callback runs on the marlin thread — keep it short and don't block. Return `false` to abort
/// the calibration (run() will then return false without touching the remaining tools).
using ProgressCallback = std::function<bool(const ProgressReport &)>;

/// Run the full tool offset calibration sequence:
/// 1. Z-offset calibration: probe a line with first tool at reference positions, interpolate other tools
/// 2. XY-offset calibration: for each tool, move to a location of tool_offset board, execute calibration moves, return
/// @param r_param (up to) millimeters of random jitter on X & Y axis during Z_probing each tool
/// @param probe_count number of Z probe repetitions per point to average
/// @param context see Context
/// @param progress_cb optional progress reporter (called once per tool)
/// @return true if calibration was successful
bool run(uint8_t r_param, uint8_t probe_count, Context context = Context::Print, const ProgressCallback &progress_cb = {});

/// Run XY offset calibration for a single tool, without touching Z offset or other tools.
/// @param tool The tool to calibrate
/// @param config The probing configuration to use
/// @param context see Context
/// @return true if calibration was successful
bool calibrate_xy_offset(PhysicalToolIndex tool, const tool_offset::ProbingConfig &config, Context context = Context::Print);

#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
/// Overwrite the sensor position in `config` with the calibrated value from the
/// config store, unless that value differs from the default by more than
/// `sensor_position_error_threshold` (in which case the default is kept and an
/// error is logged).
///
/// Single-coil only: the store holds one sensor position, while dual-coil has a
/// separate coil per axis (TODO WP5.4: per-coil storage + migration).
void apply_stored_sensor_position(tool_offset::ProbingConfig &config);
#else
/// Dual-coil counterpart of apply_stored_sensor_position: shift both coils and
/// the Z-probe spot in `config` by the stored whole-sensor displacement, clamped
/// so all shifted geometry stays within travel. A corrupt stored value (component
/// over `sensor_displacement_error_threshold`) is reset to zero.
/// @return the displacement actually applied
xy_pos_t apply_stored_sensor_displacement(tool_offset::ProbingConfig &config);
#endif

/// True iff the contactless tool-offset hardware is reachable in this
/// build/runtime configuration. The xlBuddy master image is shared between
/// plain XL (no bridge, no sensor) and XLS (bridge + sensor on dock 9), so
/// the compile-time HAS_TOOL_OFFSET_SENSOR() flag isn't sufficient on its
/// own — the sensor driver discriminates at runtime via the bridge presence
/// flag set by the puppy bootstrap.
[[nodiscard]] bool is_hardware_available();

} // namespace tool_offset_calibration
