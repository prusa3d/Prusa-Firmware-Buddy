//@file
#pragma once

#include <cstdint>

#include <indx_head/nozzle_presence.hpp>
#include <indx_head/leds.hpp>

namespace app {
void run();

/// In 1/100 °C
int16_t get_nozzle_temp_uncompensated_c100();

/// In 1/100 °C
int16_t get_nozzle_temp_compensated_c100();

/// In 1/100 °C/s
int16_t get_hotend_temp_raw_c100_dt_s();

/// PWM (0-255) representing power flowing to nozzle
uint8_t get_hotend_pwm_averaged();

/// In mW * ms. Integral of measured V × I over time.
/// Expect overflows. At 60W, overflows once in 4294967295 / 60000 / 1000000 ≈ 71 s
uint32_t get_hotend_energy_consumed_uJ();

/// In 1/100 °C
int16_t get_tpis_ambient_temp_c100();

/// @returns whether get_nozzle_temp_uncompensated_c100, and get_tpis_ambient_temp_c100 contain valid values instead of initial garbage
/// Once the temps get valid, they can only become invalid if the puppy is reset.
/// Read before the get_temp_XX to avoid race conditions.
bool get_temps_valid();

void set_nozzle_present(indx_head::NozzlePresence state);
indx_head::NozzlePresence get_nozzle_present();
void invalidate_nozzle_presence(uint16_t ack_value); ///< Reset debouncer and set nozzle state to unknown, store ack for buddy to read
uint16_t get_nozzle_invalidation_ack();

/// Last ringdown analysis decay × 1000 (unitless). 0 when latest analysis failed.
void set_ringdown_decay(int16_t value);
int16_t get_ringdown_decay();

/// Last-sampled induction heater coil current [mA]. Reads ~0 when the heater is not driving.
uint16_t get_heater_current_mA();
void set_nozzle_target_temp(uint16_t temp);
void set_led_config(const indx_head::leds::LedConfig cfg);

void set_printfan_pwm(uint8_t pwm);
uint8_t get_printfan_pwm();
uint8_t get_heatbreak_fan_pwm();

bool is_printfan_rpm_ok();
bool is_heatbreak_fan_rpm_ok();

void set_selftest_mode(bool enabled);
} // namespace app
