/// @file
#pragma once

#include <cstdint>

namespace xbuddy_extension {

enum FilamentSensorState : uint8_t {
    uninitialized,
    no_filament,
    has_filament,
    disconnected,
    _last = disconnected,
};
static constexpr uint8_t bits_per_fs_state = 2;
static_assert(FilamentSensorState::_last < (1 << bits_per_fs_state));

enum class Fan : uint8_t {
    cooling_fan_1 = 0, /// Cooling fans have shared PWM control, separate RPM readouts
    cooling_fan_2 = 1, /// Cooling fans have shared PWM control, separate RPM readouts
    filtration_fan = 2, /// Filtration fan, optional
};

static constexpr uint8_t fan_count = 3;
static constexpr uint8_t ext_filament_sensor_count = 8;

/// Represents compressed, parsed cyphal node name.
/// No need to store and compare strings all the time.
/// We wouldn't know what to do with unknown nodes anyway.
enum class NodeName : uint8_t {
    none = 0,
    cz_prusa3d_honeybee_ac_controller,
    cz_prusa3d_honeybee_nfc,
    cz_prusa3d_honeybee_tool_offset_sensor,
};

/// High-level cyphal node state for reporting.
enum class NodeState : uint8_t {
    unknown,
    verify,
    flash,
    ready,
};

/// File id used in modbus structures.
enum class FileId : uint8_t {
    none = 0,
    firmware_ac_controller,
    firmware_anfc,
    firmware_tool_offset_sensor
};

/// Status accompanying a Digest response.
enum class DigestStatus : uint8_t {
    ok = 0, ///< data contains a valid digest
    unavailable, ///< data is invalid and retry won't help
    retry, ///< data is invalid and retry might help
};

} // namespace xbuddy_extension
