#include "clo_config.hpp"

#include <option/has_indx_head.h>
#include <option/tool_offset_sensor_geometry.h>

namespace {
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
constexpr float y_shift_z_probe_offset_from_sensor = -3.2f; // See BFW-8747 geometric shift to move the probe point out of the coil area
// One physical coil, described once per axis: the Y sweep is longer than the X one.
constexpr tool_offset::CoilAxis coil_x {
    .position = { { { tool_offset::default_sensor_position.x, tool_offset::default_sensor_position.y, 0.f } } },
    .channel = tool_offset::SensorChannel::ch1,
    .sensing_distance = 6.f,
};
constexpr tool_offset::CoilAxis coil_y {
    .position = coil_x.position,
    .channel = coil_x.channel,
    .sensing_distance = 12.f,
};
#elif PRINTER_IS_PRUSA_XL()
// Expected sensor-surface height above the homed Z0, shared by both coils
constexpr float expected_surface_z = 0.4f;
// Fixed Z-probe spot: a clear area of the sensor PCB (no coil traces) that can
// take repeated loadcell probing. Common to both coils.
constexpr xyz_pos_t z_probe_position { { { -6.f, -5.f, expected_surface_z } } };
constexpr tool_offset::CoilAxis coil_x {
    .position = { { { 5.f, -6.f, expected_surface_z } } },
    .channel = tool_offset::SensorChannel::ch1,
    .sensing_distance = 16.f,
};
constexpr tool_offset::CoilAxis coil_y {
    .position = { { { -6.f, 5.f, expected_surface_z } } },
    .channel = tool_offset::SensorChannel::ch0,
    .sensing_distance = 16.f,
};
constexpr float cross_hunt_range = 5.f; // TODO tune on bench
constexpr float cross_hunt_step = 1.f; // TODO tune on bench
static_assert(z_probe_position.x >= X_MIN_POS && z_probe_position.x <= X_MAX_POS
        && z_probe_position.y >= Y_MIN_POS && z_probe_position.y <= Y_MAX_POS,
    "Z-probe spot is out of allowed travel");
static_assert(cross_hunt_step > 0.f, "zero/negative step would divide by zero in hunt_step_y");
static_assert(cross_hunt_range >= 0.f);
static_assert(2.0f * cross_hunt_range / cross_hunt_step + 1.0f <= 11.f, "hunt steps must fit the FSM iteration budget");
#else
    #error "sensor parameters not defined for this printer"
#endif
} // namespace

tool_offset::ProbingConfig tool_offset::get_default_probing_config() {
    ProbingConfig config {
        .coil_x = coil_x,
        .coil_y = coil_y,
        .safe_z_height = 4.f, // mm
        .travel_z_height = 10.f,
        .sensing_z = 0.2f,
        .sensing_speed_slow = 20.f,
        .sensing_speed_fast = 30.f,
        .sweep_rest_time = 0.35f,
        .max_safe_temp = 180.f,
        .symmetry_trim_fraction = 0.5f,
    };
#if TOOL_OFFSET_SENSOR_GEOMETRY_IS_SINGLE_COIL()
    config.y_shift_z_probe_offset_from_sensor = y_shift_z_probe_offset_from_sensor;
#else
    config.z_probe_position = z_probe_position;
    config.cross_hunt_range = cross_hunt_range;
    config.cross_hunt_step = cross_hunt_step;
#endif
    return config;
}

static_assert(coil_x.position.x - coil_x.sensing_distance / 2.0f >= X_MIN_POS, "X-sweep coil exceeds printer's physical limits");
static_assert(coil_x.position.x + coil_x.sensing_distance / 2.0f <= X_MAX_POS, "X-sweep coil exceeds printer's physical limits");
static_assert(coil_x.position.y >= Y_MIN_POS && coil_x.position.y <= Y_MAX_POS, "X-sweep coil is out of Y reach");
static_assert(coil_y.position.y - coil_y.sensing_distance / 2.0f >= Y_MIN_POS, "Y-sweep coil exceeds printer's physical limits");
static_assert(coil_y.position.y + coil_y.sensing_distance / 2.0f <= Y_MAX_POS, "Y-sweep coil exceeds printer's physical limits");
static_assert(coil_y.position.x >= X_MIN_POS && coil_y.position.x <= X_MAX_POS, "Y-sweep coil is out of X reach");
