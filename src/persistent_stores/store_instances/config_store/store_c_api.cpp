#include "store_c_api.h"
#include <bitset>
#include <config_store/store_instance.hpp>
#include <logging/log.hpp>
#include <bsod/bsod.h>
#include <option/has_15gt_belts.h>

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
    // #error dead code found by automatic analyses (see BFW-5461)
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
    // #error dead code found by automatic analyses (see BFW-5461)
    log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
#endif
}

/*****************************************************************************/
// AXIS_STEPS_PER_UNIT

namespace {

// X/Y steps/mm are stored as an override, the "unset" val (0) means "derive the value from the hardware"
float effective_axis_steps_x() {
    auto r = get_default_steps_per_unit_x_signed();

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    if (auto cs = config_store().axis_steps_per_unit_x.get(); cs != config_store_ns::steps_per_unit_unset) {
        r = cs;
    }
#endif

    return r;
}

float effective_axis_steps_y() {
    auto r = get_default_steps_per_unit_y_signed();

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    if (auto cs = config_store().axis_steps_per_unit_y.get(); cs != config_store_ns::steps_per_unit_unset) {
        r = cs;
    }
#endif

    return r;
}

float effective_axis_steps_z() {
    auto r = config_store_ns::defaults::axis_steps_per_unit_z;

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    if (auto cs = config_store().axis_steps_per_unit_z.get(); cs != config_store_ns::defaults::axis_steps_per_unit_z) {
        r = cs;
    }
#endif

    return r;
}

float effective_axis_steps_e() {
    auto r = config_store_ns::defaults::axis_steps_per_unit_e0;

    if (auto cs = config_store().axis_steps_per_unit_e0.get(); cs != config_store_ns::defaults::axis_steps_per_unit_e0) {
        r = cs;
    }

    return r;
}

} // namespace

extern "C" float get_steps_per_unit_x() {
    return std::abs(effective_axis_steps_x());
}
extern "C" float get_steps_per_unit_y() {
    return std::abs(effective_axis_steps_y());
}
extern "C" float get_steps_per_unit_z() {
    return std::abs(effective_axis_steps_z());
}
extern "C" float get_steps_per_unit_e() {
    return std::abs(effective_axis_steps_e());
}

extern "C" bool has_inverted_x() {
    return std::signbit(effective_axis_steps_x());
}

extern "C" bool has_inverted_y() {
    return std::signbit(effective_axis_steps_y());
}

extern "C" bool has_inverted_z() {
    return std::signbit(effective_axis_steps_z());
}

extern "C" bool has_inverted_e() {
    return std::signbit(effective_axis_steps_e());
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
// #error dead code found by automatic analyses (see BFW-5461)
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

// by write functions, cannot read startup variables, must read current value from eeprom

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
extern "C" void set_steps_per_unit_z(float steps) {
    if (steps > 0) {
        bool negative_direction = signbit(config_store().axis_steps_per_unit_z.get());
        config_store().axis_steps_per_unit_z.set(negative_direction ? -steps : steps);
    }
}
#endif

extern "C" void set_steps_per_unit_e(float steps) {
    if (steps > 0) {
        bool negative_direction = signbit(config_store().axis_steps_per_unit_e0.get());
        config_store().axis_steps_per_unit_e0.set(negative_direction ? -steps : steps);
    }
}

// by write functions, cannot read startup variables, must read current value from eeprom

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
static void set_positive_direction_z() {
    float steps = std::abs(config_store().axis_steps_per_unit_z.get());
    config_store().axis_steps_per_unit_z.set(steps);
}
static void set_negative_direction_z() {
    float steps = std::abs(config_store().axis_steps_per_unit_z.get());
    config_store().axis_steps_per_unit_z.set(-steps);
}
    #ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
extern "C" void set_wrong_direction_z() {
    (!DEFAULT_INVERT_Z_DIR) ? set_negative_direction_z() : set_positive_direction_z();
}
extern "C" void set_PRUSA_direction_z() {
    DEFAULT_INVERT_Z_DIR ? set_negative_direction_z() : set_positive_direction_z();
}
    #endif
#endif

static void set_positive_direction_e() {
    float steps = std::abs(config_store().axis_steps_per_unit_e0.get());
    config_store().axis_steps_per_unit_e0.set(steps);
}

static void set_negative_direction_e() {
    float steps = std::abs(config_store().axis_steps_per_unit_e0.get());
    config_store().axis_steps_per_unit_e0.set(-steps);
}

#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
extern "C" void set_wrong_direction_e() {
    (!DEFAULT_INVERT_E0_DIR) ? set_negative_direction_e() : set_positive_direction_e();
}

extern "C" void set_PRUSA_direction_e() {
    DEFAULT_INVERT_E0_DIR ? set_negative_direction_e() : set_positive_direction_e();
}
#else
// #error dead code found by automatic analyses (see BFW-5461)
extern "C" void set_wrong_direction_z() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_wrong_direction_e() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_z() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_e() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
#endif

/*****************************************************************************/
// AXIS_MICROSTEPS
bool get_has_400step_xy_motors() {
#if PRINTER_IS_PRUSA_MK4()
    return extended_printer_type_has_400step_motors[config_store().extended_printer_type.get()];
#elif PRINTER_IS_PRUSA_MINI()
    return false;
#elif PRINTER_IS_PRUSA_XL()
    return false;
#elif PRINTER_IS_PRUSA_iX()
    return false;
#elif PRINTER_IS_PRUSA_XL_DEV_KIT()
    // #error dead code found by automatic analyses (see BFW-5461)
    return false;
#elif PRINTER_IS_PRUSA_MK3_5()
    return false;
#elif PRINTER_IS_PRUSA_COREONE()
    return true;
#elif PRINTER_IS_PRUSA_COREONEL()
    return false;
#else
    #error
#endif
}

extern "C" uint16_t get_microsteps_x() {
#ifdef X_MICROSTEPS
    #if defined(X_400_STEP_MICROSTEPS) || defined(X_200_STEP_MICROSTEPS)
        #error
    #endif
    return X_MICROSTEPS;
#else
    return get_has_400step_xy_motors() ? X_400_STEP_MICROSTEPS : X_200_STEP_MICROSTEPS;
#endif
}

extern "C" uint16_t get_microsteps_y() {
#ifdef Y_MICROSTEPS
    #if defined(Y_400_STEP_MICROSTEPS) || defined(Y_200_STEP_MICROSTEPS)
        #error
    #endif
    return Y_MICROSTEPS;
#else
    return get_has_400step_xy_motors() ? Y_400_STEP_MICROSTEPS : Y_200_STEP_MICROSTEPS;
#endif
}

extern "C" uint16_t get_microsteps_z() {
    return Z_MICROSTEPS;
}

extern "C" uint16_t get_microsteps_e() {
    return E0_MICROSTEPS;
}

///@return default signed X steps/mm, depending on the belt/hardware config
extern "C" float get_default_steps_per_unit_x_signed() {
    constexpr float dir = (DEFAULT_INVERT_X_DIR == true) ? -1.f : 1.f;
#if HAS_15GT_BELTS()
    return dir * (config_store().belts_15gt_installed.get() ? AXIS_STEPS_PER_UNIT_15GT_XY : AXIS_STEPS_PER_UNIT_2GT_XY);
#else
    return dir * DEFAULT_AXIS_STEPS_PER_UNIT_X;
#endif
}

///@return default signed Y steps/mm, depending on the belt/hardware config
extern "C" float get_default_steps_per_unit_y_signed() {
    constexpr float dir = (DEFAULT_INVERT_Y_DIR == true) ? -1.f : 1.f;
#if HAS_15GT_BELTS()
    return dir * (config_store().belts_15gt_installed.get() ? AXIS_STEPS_PER_UNIT_15GT_XY : AXIS_STEPS_PER_UNIT_2GT_XY);
#else
    return dir * DEFAULT_AXIS_STEPS_PER_UNIT_Y;
#endif
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
    uint16_t r = get_default_rms_current_ma_x();

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    if (auto cs = config_store().axis_rms_current_ma_X_.get(); cs != 0) {
        r = cs;
    }
#endif

    return r;
}

extern "C" uint16_t get_rms_current_ma_y() {
    uint16_t r = get_default_rms_current_ma_y();

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    if (auto cs = config_store().axis_rms_current_ma_Y_.get(); cs != 0) {
        r = cs;
    }
#endif

    return r;
}

extern "C" uint16_t get_rms_current_ma_z() {
    uint16_t r = get_default_rms_current_ma_z();

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    if (auto cs = config_store().axis_rms_current_ma_Z_.get(); cs != 0) {
        r = cs;
    }
#endif

    return r;
}

extern "C" uint16_t get_rms_current_ma_e() {
    uint16_t r = get_default_rms_current_ma_e();

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    if (auto cs = config_store().axis_rms_current_ma_E0_.get(); cs != 0) {
        r = cs;
    }
#endif

    return r;
}

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
extern "C" void set_rms_current_ma_x(uint16_t current) {
    config_store().axis_rms_current_ma_X_.set(current);
    log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
}

extern "C" void set_rms_current_ma_y(uint16_t current) {
    config_store().axis_rms_current_ma_Y_.set(current);
    log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
}

extern "C" void set_rms_current_ma_z(uint16_t current) {
    config_store().axis_rms_current_ma_Z_.set(current);
    log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
}

extern "C" void set_rms_current_ma_e(uint16_t current) {
    config_store().axis_rms_current_ma_E0_.set(current);
    log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
}
#endif
