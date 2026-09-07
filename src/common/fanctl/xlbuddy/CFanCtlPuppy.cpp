#include "CFanCtlPuppy.hpp"

void CFanCtlPuppy::enter_selftest_mode() {
    selftest_mode = true;
#if HAS_INDX()
    tool.set_selftest_mode(true);
#endif
}

void CFanCtlPuppy::exit_selftest_mode() {
    selftest_mode = false;
#if HAS_INDX()
    tool.set_selftest_mode(false);
#endif

    // upon exit selftestt, either switch fan back to AUTO mode, or turn it off and let marlin turn it on if needed
    reset_fan();
}

void CFanCtlPuppy::reset_fan() {
    if (is_autofan) {
        tool.set_fan_auto(fan_nr);
    } else {
        tool.set_fan(fan_nr, 0);
    }
}

bool CFanCtlPuppy::set_pwm(uint16_t pwm) {
    if (selftest_mode) {
        return false;
    }

    uint16_t remapped_pwm = pwm;

#if HAS_PRINT_FAN_TYPE()
    if (fan_nr == 0) {
        PrintFanType pft = get_print_fan_type(tool.tool_index());
        remapped_pwm = print_fan_remap_pwm(pft, pwm);
    }
#endif

    tool.set_fan(fan_nr, remapped_pwm);
    return true;
}

bool CFanCtlPuppy::selftest_set_pwm(uint8_t pwm) {
    if (!selftest_mode) {
        return false;
    }

    uint16_t remapped_pwm = pwm;

#if HAS_PRINT_FAN_TYPE()
    if (fan_nr == 0) {
        PrintFanType pft = get_print_fan_type(tool.tool_index());
        remapped_pwm = print_fan_remap_pwm(pft, pwm);
    }
#endif

    tool.set_fan(fan_nr, remapped_pwm);
    return true;
}

uint8_t CFanCtlPuppy::get_pwm() const {
    return tool.get_fan_pwm(fan_nr);
}

uint16_t CFanCtlPuppy::get_actual_rpm() const {
    return tool.get_fan_rpm(fan_nr);
}

bool CFanCtlPuppy::get_rpm_is_ok() const {
    return tool.get_fan_rpm_ok(fan_nr);
}

CFanCtlCommon::FanState CFanCtlPuppy::get_state() const {
    return static_cast<FanState>(tool.get_fan_state(fan_nr));
}
