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
static float get_steps_per_unit(AxisEnum axis) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    return std::abs(config_store().printer_config.get().steps_per_unit[axis]);
}

// AXIS_STEPS_PER_UNIT
extern "C" float get_steps_per_unit_x() {
    return get_steps_per_unit(X_AXIS);
}
extern "C" float get_steps_per_unit_y() {
    return get_steps_per_unit(Y_AXIS);
}
extern "C" float get_steps_per_unit_z() {
    return get_steps_per_unit(Z_AXIS);
}
extern "C" float get_steps_per_unit_e() {
    return get_steps_per_unit(E0_AXIS);
}

static bool has_inverted_axis(AxisEnum axis) {
    return std::signbit(get_steps_per_unit(axis));
}

extern "C" bool has_inverted_x() {
    return has_inverted_axis(X_AXIS);
}

extern "C" bool has_inverted_y() {
    return has_inverted_axis(Y_AXIS);
}

extern "C" bool has_inverted_z() {
    return has_inverted_axis(Z_AXIS);
}

extern "C" bool has_inverted_e() {
    return has_inverted_axis(E_AXIS);
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

static void set_steps_per_unit(AxisEnum axis, float steps) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    if (steps > 0) {
        bool negative_dir = has_inverted_axis(axis);
        float steps_adjusted = negative_dir ? -steps : steps;
        auto steps_config = config_store().printer_config.get().steps_per_unit;
        steps_config[axis] = steps_adjusted;
        config_store().printer_config.get().set_steps_per_unit(steps_config);
    }
}

extern "C" void set_steps_per_unit_x(float steps) {
    set_steps_per_unit(X_AXIS, steps);
}
extern "C" void set_steps_per_unit_y(float steps) {
    set_steps_per_unit(Y_AXIS, steps);
}
extern "C" void set_steps_per_unit_z(float steps) {
    set_steps_per_unit(Z_AXIS, steps);
}
extern "C" void set_steps_per_unit_e(float steps) {
    set_steps_per_unit(E_AXIS, steps);
}

void set_axis_positive_direction(AxisEnum axis) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    float steps = std::abs(get_steps_per_unit(axis));
    auto steps_config = config_store().printer_config.get().steps_per_unit;
    steps_config[axis] = steps;
    config_store().printer_config.get().set_steps_per_unit(steps_config);
}

extern "C" void set_positive_direction_x() {
    set_axis_positive_direction(X_AXIS);
}
extern "C" void set_positive_direction_y() {
    set_axis_positive_direction(Y_AXIS);
}
extern "C" void set_positive_direction_z() {
    set_axis_positive_direction(Z_AXIS);
}
extern "C" void set_positive_direction_e() {
    set_axis_positive_direction(E_AXIS);
}

void set_axis_negative_direction(AxisEnum axis) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    float steps = std::abs(get_steps_per_unit(axis));
    auto steps_config = config_store().printer_config.get().steps_per_unit;
    steps_config[axis] = -steps;
    config_store().printer_config.get().set_steps_per_unit(steps_config);
}

extern "C" void set_negative_direction_x() {
    set_axis_negative_direction(X_AXIS);
}
extern "C" void set_negative_direction_y() {
    set_axis_negative_direction(Y_AXIS);
}
extern "C" void set_negative_direction_z() {
    set_axis_negative_direction(Z_AXIS);
}
extern "C" void set_negative_direction_e() {
    set_axis_negative_direction(E_AXIS);
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

static uint16_t get_microsteps(AxisEnum axis) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    return config_store().printer_config.get().microsteps[axis];
}
// return default value if eeprom value is invalid
extern "C" uint16_t get_microsteps_X() {
    uint16_t ret = get_microsteps(X_AXIS);
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = X_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_Y() {
    uint16_t ret = get_microsteps(Y_AXIS);
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Y_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_Z() {
    uint16_t ret = get_microsteps(Z_AXIS);
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Z_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_E0() {
    uint16_t ret = get_microsteps(E_AXIS);
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = E0_MICROSTEPS;
    }
    return ret;
}

void set_microsteps(uint16_t microsteps, AxisEnum axis) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    if (is_microstep_value_valid(microsteps)) {
        auto microsteps_arr = config_store().printer_config.get().get_microsteps();
        microsteps_arr[axis] = microsteps;
        config_store().printer_config.get().set_microsteps(microsteps_arr);
        log_debug(EEPROM, "%s: microsteps %d ", __PRETTY_FUNCTION__, microsteps);
    } else {
        log_error(EEPROM, "%s: microsteps %d not set", __PRETTY_FUNCTION__, microsteps);
    }
}

extern "C" void set_microsteps_x(uint16_t microsteps) {
    set_microsteps(microsteps, X_AXIS);
}
extern "C" void set_microsteps_y(uint16_t microsteps) {
    set_microsteps(microsteps, Y_AXIS);
}
extern "C" void set_microsteps_z(uint16_t microsteps) {
    set_microsteps(microsteps, Z_AXIS);
}
extern "C" void set_microsteps_e(uint16_t microsteps) {
    set_microsteps(microsteps, E_AXIS);
}

/*****************************************************************************/
// AXIS_RMS_CURRENT_MA_X
// current must be > 0, return default value if it is not
uint16_t get_rms_current_mx(AxisEnum axis) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    return config_store().printer_config.get().rms_curr[axis];
}
extern "C" uint16_t get_rms_current_ma_X() {
    uint16_t ret = get_rms_current_mx(X_AXIS);
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = X_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_Y() {
    uint16_t ret = get_rms_current_mx(Y_AXIS);
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Y_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_Z() {
    uint16_t ret = get_rms_current_mx(Z_AXIS);
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Z_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_E0() {
    uint16_t ret = get_rms_current_mx(E_AXIS);
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = E0_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}

void set_rms_current_ma(uint16_t current, AxisEnum axis) {
    if (axis >= AxisEnum::NUM_AXIS_ENUMS) {
        fatal_error("Invalid axis number", "Configuration store");
    }
    if (current > 0) {
        log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
        auto rms_currents = config_store().printer_config.get().get_microsteps();
        rms_currents[axis] = current;
        config_store().printer_config.get().set_microsteps(rms_currents);
    } else {
        log_error(EEPROM, "%s: current must be greater than 0", __PRETTY_FUNCTION__);
    }
}

extern "C" void set_rms_current_ma_x(uint16_t current) {
    set_rms_current_ma(current, X_AXIS);
}
extern "C" void set_rms_current_ma_y(uint16_t current) {
    set_rms_current_ma(current, Y_AXIS);
}
extern "C" void set_rms_current_ma_z(uint16_t current) {
    set_rms_current_ma(current, Z_AXIS);
}
extern "C" void set_rms_current_ma_e(uint16_t current) {
    set_rms_current_ma(current, E_AXIS);
}

extern "C" bool get_msc_enabled() {
    return config_store().usb_msc_enabled.get();
}
extern "C" void set_msc_enabled(bool settings) {
    config_store().usb_msc_enabled.set(settings);
}
