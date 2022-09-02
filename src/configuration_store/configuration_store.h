#pragma once
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void init_configuration_store();

/**
 * @brief function set to read value from eeprom
 */
float get_z_max_pos_mm();
float get_steps_per_unit_x();
float get_steps_per_unit_y();
float get_steps_per_unit_z();
float get_steps_per_unit_e();
uint16_t get_microsteps_X();
uint16_t get_microsteps_Y();
uint16_t get_microsteps_Z();
uint16_t get_microsteps_E0();
uint16_t get_rms_current_ma_X();
uint16_t get_rms_current_ma_Y();
uint16_t get_rms_current_ma_Z();
uint16_t get_rms_current_ma_E0();
bool has_inverted_x();
bool has_inverted_y();
bool has_inverted_z();
bool has_inverted_e();
//wrong motor direction != Prusa default
bool has_wrong_x();
bool has_wrong_y();
bool has_wrong_z();
bool has_wrong_e();
bool get_msc_enabled();
void set_msc_enabled(bool settings);

/**
 * @brief function set to read float value from eeprom and round it
 */
uint16_t get_z_max_pos_mm_rounded();
uint16_t get_steps_per_unit_x_rounded();
uint16_t get_steps_per_unit_y_rounded();
uint16_t get_steps_per_unit_z_rounded();
uint16_t get_steps_per_unit_e_rounded();

/**
 * @brief function set to store value to eeprom
 */
void set_z_max_pos_mm(float max_pos);
void set_steps_per_unit_x(float steps);
void set_steps_per_unit_y(float steps);
void set_steps_per_unit_z(float steps);
void set_steps_per_unit_e(float steps);
void set_positive_direction_x();
void set_positive_direction_y();
void set_positive_direction_z();
void set_positive_direction_e();
void set_negative_direction_x();
void set_negative_direction_y();
void set_negative_direction_z();
void set_negative_direction_e();
//wrong motor direction != Prusa default
void set_wrong_direction_x();
void set_wrong_direction_y();
void set_wrong_direction_z();
void set_wrong_direction_e();
void set_PRUSA_direction_x();
void set_PRUSA_direction_y();
void set_PRUSA_direction_z();
void set_PRUSA_direction_e();
void set_microsteps_x(uint16_t microsteps);
void set_microsteps_y(uint16_t microsteps);
void set_microsteps_z(uint16_t microsteps);
void set_microsteps_e(uint16_t microsteps);
void set_rms_current_ma_x(uint16_t current);
void set_rms_current_ma_y(uint16_t current);
void set_rms_current_ma_z(uint16_t current);
void set_rms_current_ma_e(uint16_t current);

#ifdef __cplusplus
}
#endif //__cplusplus
