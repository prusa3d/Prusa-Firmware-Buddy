#pragma once

#include <cstdint>

/**
 * @brief Shared between buddy and dwarf
 */
namespace dwarf_shared::registers {

enum class SystemDiscreteInput : uint16_t {
    is_picked = 0x0000,
    is_parked = 0x0001
};

enum class SystemCoil : uint16_t {
    tmc_enable = 0x4000,
    is_selected = 0x4001,
};

enum class SystemInputRegister : uint16_t {

    hw_bom_id = 0x8001,
    hw_otp_timestamp_0 = 0x8002, // 4 B UNIX timestamp, seconds since 1970
    hw_otp_timestamp_1 = 0x8003,
    hw_raw_datamatrix_first = 0x8004, // 24 B raw datamatrix string
    hw_raw_datamatrix_last = 0x800f,
    tmc_read_response_1 = 0x8011,
    tmc_read_response_2 = 0x8012,

    fault_status = 0x8060,
    hotend_measured_temperature = 0x8061,
    hotend_pwm_state = 0x8062,
    tool_filament_sensor = 0x8063,
    mcu_temperature = 0x8064,
    heatbreak_temp = 0x8065,
    is_picked_raw = 0x8066,
    is_parked_raw = 0x8067,
    fan0_rpm = 0x8068,
    fan0_pwm = 0x8069,
    fan0_state = 0x806A,
    fan0_is_rpm_ok = 0x806B,
    fan1_rpm = 0x806C,
    fan1_pwm = 0x806D,
    fan1_state = 0x806E,
    fan1_is_rpm_ok = 0x806F,

    time_sync_lo = 0x8070,
    time_sync_hi = 0x8071,

    marlin_error_component_start = 0x8072, // 20 characters, 10 registers
    marlin_error_component_end = 0x807B,

    marlin_error_message_start = 0x807C, // 64 characters, 32 registers
    marlin_error_message_end = 0x809B,

};

enum class SystemHoldingRegister : uint16_t {
    nozzle_target_temperature = 0xE000,
    heatbreak_requested_temperature = 0xE001,
    fan0_pwm = 0xE002,
    fan1_pwm = 0xE003,

    tmc_read_request = 0xE020,
    tmc_write_request_address = 0xE021,
    tmc_write_request_value_1 = 0xE022,
    tmc_write_request_value_2 = 0xE023,
};

enum class SystemFIFO : uint16_t {
    encoded_stream = 0x0000,
};

} // namespace dwarf_shared::registers
