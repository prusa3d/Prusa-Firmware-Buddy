//@file
#pragma once

#include <utils/storage/enum_bitset.hpp>
#include <indx_head/nozzle_presence.hpp>
#include <indx_head/leds.hpp>
#include <indx_head/errors.hpp>

#include <array>
#include <cstdint>
#include <type_traits>

// This file describes MODBUS registers for indx_head device.
// TODO: We currently mimic dwarf modbus layout, but this shouldn't be necessary.
// So feel free to move registers around as needed;

namespace indx_head::modbus {

/// Default hotend temperature in 1/100 °C used until a real reading arrives.
/// Picked to not immediately trigger mintemp.
constexpr uint16_t default_hotend_temperature_c100 = 25 * 100;

/// These data are updated regularly to reflect current status of the device.
/// Also these values are big subject to change, most of them won't be used by indx_head.
struct Status {
    static constexpr uint16_t address = 0x8060;
    errors::FaultStatusMask fault_status = indx_head::errors::FaultStatusMask::no_fault;
    int16_t hotend_measured_temperature_uncompensated_c100 = default_hotend_temperature_c100;
    int16_t hotend_measured_temperature_compensated_c100 = default_hotend_temperature_c100;

    /// In 1/100 °C/s. Derivation of the temperature measured by the hotend temp sensor
    /// Evaluated before temperature compensation
    /// !!! Does not correspond to the compensated readings
    int16_t hotend_temp_raw_c100_dt_s = 0;

    /// PWM (0-255) representing power flowing to nozzle
    uint16_t hotend_pwm_averaged = 0;

    /// Integral of measured V × I power over time [mW × ms]
    /// Expect overflows. At 60W, overflows once in 4294967295 / 60000 / 1000000 ≈ 71 s
    uint16_t hotend_energy_consumed_uJ_lo = 0;
    uint16_t hotend_energy_consumed_uJ_hi = 0;

    /// Ambient temperature reported by the TPiS sensor [int16_t in 1/100 °C]
    int16_t tpis_ambient_temperature_c100 = default_hotend_temperature_c100;

    int16_t board_temperature = 0; // [int16_t degree C]
    int16_t mcu_temperature = 0; // [int16_t degree C]
    uint16_t print_fan_rpm = 0;
    uint16_t print_fan_pwm = 0;
    uint16_t print_fan_state = 0;
    uint16_t print_fan_is_rpm_ok = 0;
    uint16_t heatbreak_fan_rpm = 0;
    uint16_t heatbreak_fan_pwm = 0;
    uint16_t heatbreak_fan_state = 0;
    uint16_t heatbreak_fan_is_rpm_ok = 0;
    uint16_t system_24V_mV = 0; // [mV]
    uint16_t time_sync_lo = 0;
    uint16_t time_sync_hi = 0;
    uint16_t nozzle_present = 0; // indx_head::NozzlePresence::unknown
    uint16_t nozzle_invalidation_ack = 0; // Echoed from xBuddy invalidate_nozzle_presence, returned after debouncer reset

    /// Whether hotend_measured_temperature_uncompensated_c100, hotend_measured_temperature_compensated_c100 and tpis_ambient_temperature_c100 values are valid.
    /// It takes a while to get these values after the head bootup, so we're initially sending garbage.
    /// Once the temps get valid, they can only become invalid if the puppy is reset.
    uint16_t temps_valid = 0;

    /// Ringdown analysis decay value × 1000 (unitless, typical range 0..200).
    /// Updated after each ringdown analysis; 0 when the latest analysis didn't have enough peaks.
    int16_t ringdown_decay = 0;

    /// Last-sampled induction heater coil current [mA]. Reads ~0 when not driving.
    uint16_t heater_current_mA = 0;
};
static_assert(sizeof(Status) % 2 == 0);
static_assert(std::alignment_of_v<Status> == 2);

/// These data are used to configure the device. Again some of the values might not be used, so feel free to change them (again only for initial Dwarf support).
struct Config {
    static constexpr uint16_t address = 0xE000;
    uint16_t nozzle_target_temperature = 0;
    struct {
        uint8_t value = 0;
        uint8_t padding = 0;
    } print_fan_pwm;
    indx_head::leds::LedConfig leds;
    int16_t hotend_temperature_compensation_c100 = 0; // In 1/100 °C
    uint16_t invalidate_nozzle_presence = 0;
    uint16_t loadcell_enabled = 0;
    uint16_t accelerometer_enabled = 0;
    uint16_t clear_fault_status = 0; ///< errors::FaultStatusMask to clear from Status::fault_status when written
    uint16_t selftest_mode = 0; ///< When nonzero, heatbreak fan is forced to full speed for selftest
};
static_assert(sizeof(Config) % 2 == 0);
static_assert(std::alignment_of_v<Config> == 2);
} // namespace indx_head::modbus
