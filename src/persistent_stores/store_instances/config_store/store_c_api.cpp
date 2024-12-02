#include "store_c_api.h"
#include <bitset>
#include <config_store/store_instance.hpp>
#include <logging/log.hpp>

LOG_COMPONENT_DEF(EEPROM, logging::Severity::info);

extern "C" float get_z_max_pos_mm() {
    float ret = 0.F;
#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    ret = config_store().axis_z_max_pos_mm.get();
    if ((ret > Z_MAX_LEN_LIMIT) || (ret < Z_MIN_LEN_LIMIT)) {
        ret = DEFAULT_Z_MAX_POS;
    }
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
        config_store().axis_z_max_pos_mm.set(max_pos);
    }
    log_debug(EEPROM, "%s set %f", __PRETTY_FUNCTION__, double(max_pos));
#else
    log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
#endif
}

/*****************************************************************************/
// AXIS_STEPS_PER_UNIT
extern "C" float get_steps_per_unit_x() {
    return std::abs(config_store().axis_steps_per_unit_x.get());
}
extern "C" float get_steps_per_unit_y() {
    return std::abs(config_store().axis_steps_per_unit_y.get());
}
extern "C" float get_steps_per_unit_z() {
    return std::abs(config_store().axis_steps_per_unit_z.get());
}
extern "C" float get_steps_per_unit_e() {
    return std::abs(config_store().axis_steps_per_unit_e0.get());
}

extern "C" bool has_inverted_x() {
    return std::signbit(config_store().axis_steps_per_unit_x.get());
}

extern "C" bool has_inverted_y() {
    return std::signbit(config_store().axis_steps_per_unit_y.get());
}

extern "C" bool has_inverted_z() {
    return std::signbit(config_store().axis_steps_per_unit_z.get());
}

extern "C" bool has_inverted_e() {
    return std::signbit(config_store().axis_steps_per_unit_e0.get());
}

extern "C" bool has_inverted_axis(const uint8_t axis) {
    switch (axis) {
    case X_AXIS:
        return has_inverted_x();
    case Y_AXIS:
        return has_inverted_y();
    case Z_AXIS:
        return has_inverted_z();
    case E_AXIS:
        return has_inverted_e();
    default:
        bsod("invalid axis");
    }
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

extern "C" bool get_print_area_based_heating_enabled() {
    return config_store().heat_entire_bed.get() == false;
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

// by write functions, cannot read startup variables, must read current value from eeprom
extern "C" void set_steps_per_unit_x(float steps) {
    if (steps > 0) {
        bool negative_direction = signbit(config_store().axis_steps_per_unit_x.get());
        config_store().axis_steps_per_unit_x.set(negative_direction ? -steps : steps);
    }
}
extern "C" void set_steps_per_unit_y(float steps) {
    if (steps > 0) {
        bool negative_direction = signbit(config_store().axis_steps_per_unit_y.get());
        config_store().axis_steps_per_unit_y.set(negative_direction ? -steps : steps);
    }
}
extern "C" void set_steps_per_unit_z(float steps) {
    if (steps > 0) {
        bool negative_direction = signbit(config_store().axis_steps_per_unit_z.get());
        config_store().axis_steps_per_unit_z.set(negative_direction ? -steps : steps);
    }
}
extern "C" void set_steps_per_unit_e(float steps) {
    if (steps > 0) {
        bool negative_direction = signbit(config_store().axis_steps_per_unit_e0.get());
        config_store().axis_steps_per_unit_e0.set(negative_direction ? -steps : steps);
    }
}

// by write functions, cannot read startup variables, must read current value from eeprom
static void set_positive_direction_x() {
    float steps = std::abs(config_store().axis_steps_per_unit_x.get());
    config_store().axis_steps_per_unit_x.set(steps);
}
static void set_positive_direction_y() {
    float steps = std::abs(config_store().axis_steps_per_unit_y.get());
    config_store().axis_steps_per_unit_y.set(steps);
}
static void set_positive_direction_z() {
    float steps = std::abs(config_store().axis_steps_per_unit_z.get());
    config_store().axis_steps_per_unit_z.set(steps);
}
static void set_positive_direction_e() {
    float steps = std::abs(config_store().axis_steps_per_unit_e0.get());
    config_store().axis_steps_per_unit_e0.set(steps);
}

static void set_negative_direction_x() {
    float steps = std::abs(config_store().axis_steps_per_unit_x.get());
    config_store().axis_steps_per_unit_x.set(-steps);
}
static void set_negative_direction_y() {
    float steps = std::abs(config_store().axis_steps_per_unit_y.get());
    config_store().axis_steps_per_unit_y.set(-steps);
}
static void set_negative_direction_z() {
    float steps = std::abs(config_store().axis_steps_per_unit_z.get());
    config_store().axis_steps_per_unit_z.set(-steps);
}
static void set_negative_direction_e() {
    float steps = std::abs(config_store().axis_steps_per_unit_e0.get());
    config_store().axis_steps_per_unit_e0.set(-steps);
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

bool get_has_400step_xy_motors() {
#if PRINTER_IS_PRUSA_MK4()
    return extended_printer_type_has_400step_motors[config_store().extended_printer_type.get()];
#else
    return false;
#endif
}

///@return default microstep value depending on motor type config
extern "C" uint16_t get_default_microsteps_x() {
#ifdef X_MICROSTEPS
    return X_MICROSTEPS;
#else
    return get_has_400step_xy_motors() ? X_400_STEP_MICROSTEPS : X_200_STEP_MICROSTEPS;
#endif
}

///@return default microstep value depending on motor type config
extern "C" uint16_t get_default_microsteps_y() {
#ifdef Y_MICROSTEPS
    return Y_MICROSTEPS;
#else
    return get_has_400step_xy_motors() ? Y_400_STEP_MICROSTEPS : Y_200_STEP_MICROSTEPS;
#endif
}

///@return default microstep value
extern "C" uint16_t get_default_microsteps_z() {
    return Z_MICROSTEPS;
}

///@return default microstep value
extern "C" uint16_t get_default_microsteps_e() {
    return E0_MICROSTEPS;
}

// return default value if eeprom value is invalid
extern "C" uint16_t get_microsteps_x() {
    uint16_t ret = config_store().axis_microsteps_X_.get();
    if (!is_microstep_value_valid(ret)) {
        if (ret != 0) { // 0 means use default
            log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        }
        ret = get_default_microsteps_x();
    }
    return ret;
}
extern "C" uint16_t get_microsteps_y() {
    uint16_t ret = config_store().axis_microsteps_Y_.get();
    if (!is_microstep_value_valid(ret)) {
        if (ret != 0) { // 0 means use default
            log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        }
        ret = get_default_microsteps_y();
    }
    return ret;
}
extern "C" uint16_t get_microsteps_z() {
    uint16_t ret = config_store().axis_microsteps_Z_.get();
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = get_default_microsteps_z();
    }
    return ret;
}
extern "C" uint16_t get_microsteps_e() {
    uint16_t ret = config_store().axis_microsteps_E0_.get();
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = get_default_microsteps_e();
    }
    return ret;
}

extern "C" uint16_t get_default_rms_current_ma_x() {
#ifdef X_CURRENT
    return X_CURRENT;
#else
    return get_has_400step_xy_motors() ? X_400_STEP_CURRENT : X_200_STEP_CURRENT;
#endif
}
extern "C" uint16_t get_default_rms_current_ma_y() {
#ifdef Y_CURRENT
    return Y_CURRENT;
#else
    return get_has_400step_xy_motors() ? Y_400_STEP_CURRENT : Y_200_STEP_CURRENT;
#endif
}
extern "C" uint16_t get_default_rms_current_ma_z() {
    return Z_CURRENT;
}
extern "C" uint16_t get_default_rms_current_ma_e() {
    return E0_CURRENT;
}

extern "C" uint16_t get_rms_current_ma_x() {
    uint16_t ret = config_store().axis_rms_current_ma_X_.get();
    if (ret == 0) {
        ret = get_default_rms_current_ma_x();
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_y() {
    uint16_t ret = config_store().axis_rms_current_ma_Y_.get();
    if (ret == 0) {
        ret = get_default_rms_current_ma_y();
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_z() {
    uint16_t ret = config_store().axis_rms_current_ma_Z_.get();
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = get_default_rms_current_ma_z();
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_e() {
    uint16_t ret = config_store().axis_rms_current_ma_E0_.get();
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = get_default_rms_current_ma_e();
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}

extern "C" void set_rms_current_ma_x(uint16_t current) {
    config_store().axis_rms_current_ma_X_.set(current);
    log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
}
extern "C" void set_rms_current_ma_y(uint16_t current) {
    config_store().axis_rms_current_ma_Y_.set(current);
    log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
}
extern "C" void set_rms_current_ma_z(uint16_t current) {
    if (current > 0) {
        config_store().axis_rms_current_ma_Z_.set(current);
        log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
    } else {
        log_error(EEPROM, "%s: current must be greater than 0", __PRETTY_FUNCTION__);
    }
}
extern "C" void set_rms_current_ma_e(uint16_t current) {
    if (current > 0) {
        config_store().axis_rms_current_ma_E0_.set(current);
        log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
    } else {
        log_error(EEPROM, "%s: current must be greater than 0", __PRETTY_FUNCTION__);
    }
}
