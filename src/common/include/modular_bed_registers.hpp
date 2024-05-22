#pragma once

#include <cstdint>

/**
 * @brief Shared between buddy and modular bed
 */
namespace modular_bed_shared::registers {

enum class SystemDiscreteInput : uint16_t {
    power_painc_status = 0x0000,
    current_fault_status = 0x0001,

    _post_last,
    _last = _post_last - 1,
    _first = power_painc_status,
};

enum class HBDiscreteInput : uint16_t {
    is_ready = 0x2000,

    _post_last,
    _last = _post_last - 1,
    _first = is_ready,
};

enum class SystemCoil : uint16_t {
    clear_fault_status = 0x4000,
    reset_overcurrent_fault = 0x4001,
    test_hb_heating = 0x4002,
    measure_hb_currents = 0x4003,
    calibrate_thermistors = 0x4004,
    print_fan_active = 0x4005,

    _post_last,
    _last = _post_last - 1,
    _first = clear_fault_status,
};

enum class HBCoil : uint16_t {
    clear_fault_status = 0x6000,

    _post_last,
    _last = _post_last - 1,
    _first = clear_fault_status,
};

enum class SystemInputRegister : uint16_t {
    fault_status = 0x8000,
    hw_bom_id = 0x8001,
    hw_otp_timestamp_0 = 0x8002, // 4 B UNIX timestamp, seconds since 1970
    hw_otp_timestamp_1 = 0x8003,
    hw_raw_datamatrix_first = 0x8004, // 24 B raw datamatrix string
    hw_raw_datamatrix_last = 0x800f,
    heatbedlet_count = 0x8010,
    adc_current_1 = 0x8011,
    adc_current_2 = 0x8012,
    expected_current_1 = 0x8013,
    expected_current_2 = 0x8014,
    mcu_temperature = 0x8015,

    _post_last,
    _last = _post_last - 1,
    _first = fault_status,
};

enum class HBInputRegister : uint16_t {
    fault_status = 0xA000,
    measured_temperature = 0xA010,
    pwm_state = 0xA020,
    pid_p_control_action = 0xA030,
    pid_i_control_action = 0xA040,
    pid_d_control_action = 0xA050,
    pid_tc_control_action = 0xA060,

    _post_last,
    _last = _post_last - 1,
    _first = fault_status,
};

enum class SystemHoldingRegister : uint16_t {
    chamber_temperature = 0xC000,
    clear_system_fault_bits = 0xC001,

    _post_last,
    _last = _post_last - 1,
    _first = chamber_temperature,
};

enum class HBHoldingRegister : uint16_t {
    max_allowed_current = 0xE000,
    target_temperature = 0xE010,
    measured_max_current = 0xE020,
    pid_p = 0xE030,
    pid_i = 0xE040,
    pid_d = 0xE050,
    pid_tc = 0xE060,
    clear_hb_fault_bits = 0xE070,

    _post_last,
    _last = _post_last - 1,
    _first = max_allowed_current,
};

} // namespace modular_bed_shared::registers
