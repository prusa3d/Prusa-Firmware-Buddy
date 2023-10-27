#pragma once

#include <cstdint>

/**
 * @brief Shared between buddy and dwarf
 */
namespace dwarf_shared::registers {

enum class SystemDiscreteInput : uint16_t {
    is_picked = 0x0000,
    is_parked = 0x0001,
    is_button_up_pressed = 0x0002,
    is_button_down_pressed = 0x0003,

    _post_last,
    _last = _post_last - 1,
    _first = is_picked,
};

enum class SystemCoil : uint16_t {
    tmc_enable = 0x4000,
    is_selected = 0x4001,
    loadcell_enable = 0x4002,
    accelerometer_enable = 0x4003,
    accelerometer_high = 0x4004,

    _post_last,
    _last = _post_last - 1,
    _first = tmc_enable,
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
    board_temperature = 0x8064, // [int16_t degree C]
    mcu_temperature = 0x8065, // [int16_t degree C]
    heatbreak_temp = 0x8066,
    is_picked_raw = 0x8067,
    is_parked_raw = 0x8068,
    fan0_rpm = 0x8069,
    fan0_pwm = 0x806a,
    fan0_state = 0x806b,
    fan0_is_rpm_ok = 0x806c,
    fan1_rpm = 0x806d,
    fan1_pwm = 0x806e,
    fan1_state = 0x806f,
    fan1_is_rpm_ok = 0x8070,
    system_24V_mV = 0x8071, // [mV]
    heater_current_mA = 0x8072, // [mA]

    time_sync_lo = 0x8073,
    time_sync_hi = 0x8074,

    marlin_error_component_start = 0x8075, // 20 characters, 10 registers
    marlin_error_component_end = 0x807e,

    marlin_error_message_start = 0x807f, // 64 characters, 32 registers
    marlin_error_message_end = 0x809e,

    _post_last,
    _last = _post_last - 1,
    _first = hw_bom_id,
};

enum class SystemHoldingRegister : uint16_t {
    nozzle_target_temperature = 0xE000,
    heatbreak_requested_temperature = 0xE001,
    fan0_pwm = 0xE002,
    fan1_pwm = 0xE003,
    led_pwm = 0xE004, // 8 MSb PWM when selected, 8 LSb PWM when not selected [0 - 0xff]
    status_color_start = 0xE005, // 8 MSb Green [0 - 0xff], 8 LSb Red [0 - 0xff]
    status_color_end = 0xE006, // 8 MSb status_led_mode, 8 LSb Blue [0 - 0xff]
    pid_start = 0xE007, // P, I and D float values
    pid_end = 0xE00C,

    tmc_read_request = 0xE020,
    tmc_write_request_address = 0xE021,
    tmc_write_request_value_1 = 0xE022,
    tmc_write_request_value_2 = 0xE023,

    _post_last,
    _last = _post_last - 1,
    _first = nozzle_target_temperature,
};

enum class SystemFIFO : uint16_t {
    encoded_stream = 0x0000,

    _post_last,
    _last = _post_last - 1,
    _first = encoded_stream,
};

} // namespace dwarf_shared::registers
