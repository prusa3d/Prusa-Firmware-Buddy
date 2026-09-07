/**
 * @file store_c_api.h
 * @brief api allowing access to eeprom via functions
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <option/has_extra_experimental_settings.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * @brief function set to read value from eeprom
 */
float get_z_max_pos_mm();
float get_steps_per_unit_x();
float get_steps_per_unit_y();
float get_steps_per_unit_z();
float get_steps_per_unit_e();
bool get_has_400step_xy_motors();
float get_default_steps_per_unit_x_signed();
float get_default_steps_per_unit_y_signed();
uint16_t get_microsteps_x();
uint16_t get_microsteps_y();
uint16_t get_microsteps_z();
uint16_t get_microsteps_e();
uint16_t get_rms_current_ma_x();
uint16_t get_rms_current_ma_y();
uint16_t get_rms_current_ma_z();
uint16_t get_rms_current_ma_e();
uint16_t get_default_rms_current_ma_x();
uint16_t get_default_rms_current_ma_y();
uint16_t get_default_rms_current_ma_z();
uint16_t get_default_rms_current_ma_e();
bool has_inverted_x();
bool has_inverted_y();
bool has_inverted_z();
bool has_inverted_e();
bool has_inverted_axis(const uint8_t axis);
// wrong motor direction != Prusa default
bool has_wrong_x();
bool has_wrong_y();
bool has_wrong_z();
bool has_wrong_e();
bool get_print_area_based_heating_enabled();

/**
 * @brief function set to read float value from eeprom and round it
 */
uint16_t get_z_max_pos_mm_rounded();

/**
 * @brief function set to store value to eeprom
 */
void set_z_max_pos_mm(float max_pos);

// wrong motor direction != Prusa default

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
void set_steps_per_unit_z(float steps);
void set_wrong_direction_z();
void set_PRUSA_direction_z();
#endif

void set_steps_per_unit_e(float steps);

void set_wrong_direction_e();
void set_PRUSA_direction_e();

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
void set_rms_current_ma_x(uint16_t current);
void set_rms_current_ma_y(uint16_t current);
void set_rms_current_ma_z(uint16_t current);
void set_rms_current_ma_e(uint16_t current);
#endif

#ifdef __cplusplus
}
#endif //__cplusplus
