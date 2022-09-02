#include "configuration_store.hpp"
#include "configuration_store.h"
#include "logging/log.h"
using namespace configuration_store;

LOG_COMPONENT_REF(EEPROM);

extern "C" void init_configuration_store() {
    config_store().init();
}
extern "C" float get_z_max_pos_mm() {
    float ret = 0.F;
#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    ret = config_store().printer_config.get().z_max_pos;
    if ((ret > Z_MAX_LEN_LIMIT) || (ret < Z_MIN_LEN_LIMIT))
        ret = DEFAULT_Z_MAX_POS;
    log_debug(EEPROM, "%s returned %f", __PRETTY_FUNCTION__, double(ret));
#else
    log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
#endif
    return ret;
}

extern "C" uint16_t get_z_max_pos_mm_rounded() {
    return static_cast<uint16_t>(std::lround(get_z_max_pos_mm()));
}

extern "C" void set_z_max_pos_mm(float max_pos) {
#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    if ((max_pos >= Z_MIN_LEN_LIMIT) && (max_pos <= Z_MAX_LEN_LIMIT)) {
        config_store().printer_config.get().set_z_max_pos(max_pos);
    }
    log_debug(EEPROM, "%s set %f", __PRETTY_FUNCTION__, double(max_pos));
#else
    log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
#endif
}
// AXIS_STEPS_PER_UNIT
extern "C" float get_steps_per_unit_x() {
    return std::abs(config_store().printer_config.get().steps_per_unit_x);
}
extern "C" float get_steps_per_unit_y() {
    return std::abs(config_store().printer_config.get().steps_per_unit_y);
}
extern "C" float get_steps_per_unit_z() {
    return std::abs(config_store().printer_config.get().steps_per_unit_z);
}
extern "C" float get_steps_per_unit_e() {
    return std::abs(config_store().printer_config.get().steps_per_unit_e);
}

extern "C" bool has_inverted_x() {
    return std::signbit(config_store().printer_config.get().microsteps_x);
}

extern "C" bool has_inverted_y() {
    return std::signbit(config_store().printer_config.get().steps_per_unit_y);
}

extern "C" bool has_inverted_z() {
    return std::signbit(config_store().printer_config.get().steps_per_unit_z);
}

extern "C" bool has_inverted_e() {
    return std::signbit(config_store().printer_config.get().steps_per_unit_e);
}

#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
extern "C" bool has_wrong_x() {
    return has_inverted_x() != DEFAULT_INVERT_X_DIR;
}

extern "C" bool has_wrong_y() {
    return has_inverted_y() != DEFAULT_INVERT_Y_DIR;
}

extern "C" bool has_wrong_z() {
    return has_inverted_z() != DEFAULT_INVERT_Z_DIR;
}

extern "C" bool has_wrong_e() {
    return has_inverted_e() != DEFAULT_INVERT_E0_DIR;
}
#else
extern "C" bool has_wrong_x() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
extern "C" bool has_wrong_y() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
extern "C" bool has_wrong_z() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
extern "C" bool has_wrong_e() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
#endif

extern "C" uint16_t get_steps_per_unit_x_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_x()));
}
extern "C" uint16_t get_steps_per_unit_y_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_y()));
}
extern "C" uint16_t get_steps_per_unit_z_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_z()));
}
extern "C" uint16_t get_steps_per_unit_e_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_e()));
}

bool is_current_axis_value_inverted(MemConfigItem<float> &item) {
    return std::signbit(item.get());
}

void set_steps_per_unit(float steps, MemConfigItem<float> &item) {
    if (steps > 0) {
        bool negative_direction = is_current_axis_value_inverted(item);
        item.set(negative_direction ? -steps : steps);
    }
}

extern "C" void set_steps_per_unit_x(float steps) {
    if (steps > 0) {
        bool negative_direction = std::signbit(config_store().printer_config.get().steps_per_unit_x);
        config_store().printer_config.get().set_steps_per_unit_x(negative_direction ? -steps : steps);
    }
}
extern "C" void set_steps_per_unit_y(float steps) {
    if (steps > 0) {
        bool negative_direction = std::signbit(config_store().printer_config.get().steps_per_unit_y);
        config_store().printer_config.get().set_steps_per_unit_y(negative_direction ? -steps : steps);
    }
}
extern "C" void set_steps_per_unit_z(float steps) {
    if (steps > 0) {
        bool negative_direction = std::signbit(config_store().printer_config.get().steps_per_unit_z);
        config_store().printer_config.get().set_steps_per_unit_z(negative_direction ? -steps : steps);
    }
}
extern "C" void set_steps_per_unit_e(float steps) {
    if (steps > 0) {
        bool negative_direction = std::signbit(config_store().printer_config.get().steps_per_unit_e);
        config_store().printer_config.get().set_steps_per_unit_e(negative_direction ? -steps : steps);
    }
}

float get_current_steps_per_unit(MemConfigItem<float> &item) {
    return std::abs(item.get());
}

void set_axis_positive_direction(MemConfigItem<float> &item) {
    float steps = get_current_steps_per_unit(item);
    item.set(steps);
}

extern "C" void set_positive_direction_x() {
    set_axis_positive_direction(config_store().steps_per_unit_x);
}
extern "C" void set_positive_direction_y() {
    set_axis_positive_direction(config_store().steps_per_unit_y);
}
extern "C" void set_positive_direction_z() {
    set_axis_positive_direction(config_store().steps_per_unit_z);
}
extern "C" void set_positive_direction_e() {
    set_axis_positive_direction(config_store().steps_per_unit_e);
}

void set_axis_negative_direction(MemConfigItem<float> &item) {
    float steps = get_current_steps_per_unit(item);
    item.set(-steps);
}

extern "C" void set_negative_direction_x() {
    set_axis_negative_direction(config_store().steps_per_unit_x);
}
extern "C" void set_negative_direction_y() {
    set_axis_negative_direction(config_store().steps_per_unit_y);
}

extern "C" void set_negative_direction_z() {
    set_axis_negative_direction(config_store().steps_per_unit_z);
}
extern "C" void set_negative_direction_e() {
    set_axis_negative_direction(config_store().steps_per_unit_e);
}

#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
extern "C" void set_wrong_direction_x() {
    (!DEFAULT_INVERT_X_DIR) ? set_negative_direction_x() : set_positive_direction_x();
}
extern "C" void set_wrong_direction_y() {
    (!DEFAULT_INVERT_Y_DIR) ? set_negative_direction_y() : set_positive_direction_y();
}
extern "C" void set_wrong_direction_z() {
    (!DEFAULT_INVERT_Z_DIR) ? set_negative_direction_z() : set_positive_direction_z();
}
extern "C" void set_wrong_direction_e() {
    (!DEFAULT_INVERT_E0_DIR) ? set_negative_direction_e() : set_positive_direction_e();
}
extern "C" void set_PRUSA_direction_x() {
    DEFAULT_INVERT_X_DIR ? set_negative_direction_x() : set_positive_direction_x();
}
extern "C" void set_PRUSA_direction_y() {
    DEFAULT_INVERT_Y_DIR ? set_negative_direction_y() : set_positive_direction_y();
}
extern "C" void set_PRUSA_direction_z() {
    DEFAULT_INVERT_Z_DIR ? set_negative_direction_z() : set_positive_direction_z();
}
extern "C" void set_PRUSA_direction_e() {
    DEFAULT_INVERT_E0_DIR ? set_negative_direction_e() : set_positive_direction_e();
}
#else
extern "C" void set_wrong_direction_x() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_wrong_direction_y() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_wrong_direction_z() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_wrong_direction_e() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_x() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_y() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_z() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_e() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
#endif

/*****************************************************************************/
// AXIS_MICROSTEPS
bool is_microstep_value_valid(uint16_t microsteps) {
    std::bitset<16> bs(microsteps);
    return bs.count() == 1; // 1,2,4,8...
}

// return default value if eeprom value is invalid
extern "C" uint16_t get_microsteps_X() {
    uint16_t ret = config_store().microsteps_x.get();
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = X_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_Y() {
    uint16_t ret = config_store().microsteps_y.get();
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Y_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_Z() {
    uint16_t ret = config_store().microsteps_z.get();
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Z_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_E0() {
    uint16_t ret = config_store().microsteps_e.get();
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = E0_MICROSTEPS;
    }
    return ret;
}

void set_microsteps(uint16_t microsteps, MemConfigItem<uint16_t> item) {
    if (is_microstep_value_valid(microsteps)) {
        item.set(microsteps);
        log_debug(EEPROM, "%s: microsteps %d ", __PRETTY_FUNCTION__, microsteps);
    } else {
        log_error(EEPROM, "%s: microsteps %d not set", __PRETTY_FUNCTION__, microsteps);
    }
}

extern "C" void set_microsteps_x(uint16_t microsteps) {
    set_microsteps(microsteps, config_store().microsteps_x);
}
extern "C" void set_microsteps_y(uint16_t microsteps) {
    set_microsteps(microsteps, config_store().microsteps_y);
}
extern "C" void set_microsteps_z(uint16_t microsteps) {
    set_microsteps(microsteps, config_store().microsteps_z);
}
extern "C" void set_microsteps_e(uint16_t microsteps) {
    set_microsteps(microsteps, config_store().microsteps_e);
}

/*****************************************************************************/
// AXIS_RMS_CURRENT_MA_X
// current must be > 0, return default value if it is not
extern "C" uint16_t get_rms_current_ma_X() {
    uint16_t ret = config_store().rms_curr_ma_x.get();
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = X_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_Y() {
    uint16_t ret = config_store().rms_curr_ma_y.get();
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Y_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_Z() {
    uint16_t ret = config_store().rms_curr_ma_z.get();
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Z_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_E0() {
    uint16_t ret = config_store().rms_curr_ma_e.get();
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = E0_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}

void set_rms_current_ma(uint16_t current, MemConfigItem<uint16_t> item) {
    if (current > 0) {
        item.set(current);
        log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
    } else {
        log_error(EEPROM, "%s: current must be greater than 0", __PRETTY_FUNCTION__);
    }
}

extern "C" void set_rms_current_ma_x(uint16_t current) {
    set_rms_current_ma(current, config_store().rms_curr_ma_x);
}
extern "C" void set_rms_current_ma_y(uint16_t current) {
    set_rms_current_ma(current, config_store().rms_curr_ma_y);
}
extern "C" void set_rms_current_ma_z(uint16_t current) {
    set_rms_current_ma(current, config_store().rms_curr_ma_z);
}
extern "C" void set_rms_current_ma_e(uint16_t current) {
    set_rms_current_ma(current, config_store().rms_curr_ma_e);
}

extern "C" bool get_msc_enabled() {
    return config_store().usb_msc_enabled.get();
}
extern "C" void set_msc_enabled(bool settings) {
    config_store().usb_msc_enabled.set(settings);
}
