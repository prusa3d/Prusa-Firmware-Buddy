/// @file
#pragma once

#include <cstdint>

/// This file defines MODBUS register files, to be shared between client and server.
/// Resist the temptation to make this type-safe in any way! This is only used for
/// memory layout and should consist of 16-bit values, arrays and structures of such.
/// To ensure proper synchronization, you must always read/write entire register files.

namespace tool_offset_sensor::modbus {

/// MODBUS register file for reporting current status of tool_offset_sensor to xBuddy.
struct Status {
    static constexpr uint16_t address = 0x8000;

    uint16_t channel_flags; ///< bitfield, see channel_flag_* constants
    uint16_t sensor_errors; ///< sticky LDC1612 error flags
};

/// MODBUS register file for controlling the tool offset sensor from xBuddy.
struct Config {
    static constexpr uint16_t address = 0x9000;

    uint16_t ch0_enabled; ///< enable channel 0 (boolean)
    uint16_t ch1_enabled; ///< enable channel 1 (boolean)
};

// Bit positions for Status::channel_flags
static constexpr uint16_t channel_flag_ch0_active = 1 << 0;
static constexpr uint16_t channel_flag_ch1_active = 1 << 1;
static constexpr uint16_t channel_flag_sensor_fault = 1 << 2;

} // namespace tool_offset_sensor::modbus
