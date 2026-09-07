#pragma once

#include <core/types.h>
#include <config.h>
#include <printers.h>
#include <option/tool_offset_sensor_geometry.h>

#include <cstdint>

namespace tool_offset {

inline constexpr xy_pos_t default_sensor_position =
#if PRINTER_IS_PRUSA_COREONE()
    { 257.f, Y_MAX_PRINT_POS - 197.5f };
#elif PRINTER_IS_PRUSA_COREONEL()
    { 307.f, 5.f };
#elif PRINTER_IS_PRUSA_XL()
    // Dual-coil takes its positions from the coils themselves, so this only
    // survives as the config-store default, which XL never reads back.
    { 5.f, -7.f };
#else
    #error "No default probing config for this printer"
#endif

/// Which LDC1612 channel feeds a coil. On XLS the two coils sit on separate
/// channels (CH1 for the X-sweep coil, CH0 for the Y-sweep coil, as measured on
/// hardware); INDX uses CH1.
enum class SensorChannel : uint8_t { ch0 = 0,
    ch1 = 1,
};

/// The coil one axis is swept over, and how. Dual-coil (XLS) has one per axis;
/// single-coil describes the same physical coil twice, once per axis, because
/// the two sweeps differ in length.
struct CoilAxis {
    xyz_pos_t position {}; ///< coil centre, machine frame; .z is the expected sensor-surface height
    SensorChannel channel = SensorChannel::ch0; ///< LDC1612 channel feeding this coil
    float sensing_distance = 0.f; ///< sweep length along the measured axis
};

struct ProbingConfig {
    CoilAxis coil_x; ///< coil swept along X
    CoilAxis coil_y; ///< coil swept along Y (single-coil: the same coil as coil_x)

#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
    float y_shift_z_probe_offset_from_sensor;
#else
    xyz_pos_t z_probe_position;
    /// Cross-X hunt for a low-confidence Y sweep: the sweep line's X is stepped
    /// across ±cross_hunt_range around coil_y.position.x in cross_hunt_step
    /// decrements.
    float cross_hunt_range;
    float cross_hunt_step;
#endif

    float safe_z_height; // Height above the sensor for the descent before probing and the post-scan lift
    float travel_z_height; // Clearance height for XY travel to the sensor
    float sensing_z; // Height above the sensor to actually perform the measurement
    float sensing_speed_slow;
    float sensing_speed_fast;
    float sweep_rest_time; // Pause between sweep passes (seconds)
    float max_safe_temp; // Maximum nozzle temperature allowed for probing
    float symmetry_trim_fraction; // Per-pass second-correlation: keep this central fraction
                                  // (around the first-pass symmetry axis) and re-correlate.
                                  // 1.0 disables, 0.5 keeps central 50%.

    static constexpr float sensor_position_update_threshold = 0.2f;
    static constexpr float sensor_position_error_threshold = 3.0f;

    /// Dual-coil (XLS): error threshold for the whole-sensor displacement.
    /// Sensor maybe moved by 30mm for silicon head calibration, plus some margin
    static constexpr float sensor_displacement_error_threshold = 35.0f;
};

ProbingConfig get_default_probing_config();

#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
/// Move the sensor to `pos`. Both CoilAxis describe one physical coil here, so
/// a calibrated position has to land in both of them. Not available on
/// dual-coil, where the two coils move independently.
inline void set_single_coil_position(ProbingConfig &config, xy_pos_t pos) {
    config.coil_x.position.x = config.coil_y.position.x = pos.x;
    config.coil_x.position.y = config.coil_y.position.y = pos.y;
}
#endif

} // namespace tool_offset
