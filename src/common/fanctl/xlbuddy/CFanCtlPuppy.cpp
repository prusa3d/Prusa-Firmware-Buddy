#include "CFanCtlPuppy.hpp"
#include "puppies/Dwarf.hpp"

void CFanCtlPuppy::enterSelftestMode() {
    selftest_mode = true;
}

void CFanCtlPuppy::exitSelftestMode() {
    selftest_mode = false;

    // upon exit selftestt, either switch fan back to AUTO mode, or turn it off and let marlin turn it on if needed
    reset_fan();
}

void CFanCtlPuppy::reset_fan() {
    setPWM(is_autofan ? buddy::puppies::Dwarf::FAN_MODE_AUTO_PWM : 0);
}

bool CFanCtlPuppy::setPWM(uint16_t pwm) {
    if (selftest_mode) {
        return false;
    }

    buddy::puppies::dwarfs[dwarf_nr].set_fan(fan_nr, pwm);
    return true;
}

bool CFanCtlPuppy::selftestSetPWM(uint8_t pwm) {
    if (!selftest_mode) {
        return false;
    }

    uint16_t remapped_pwm = pwm;

#if HAS_PRINT_FAN_TYPE()
    if (fan_nr == 0) {
        PrintFanType pft = get_print_fan_type(dwarf_nr);
        remapped_pwm = print_fan_remap_pwm(pft, pwm);
    }
#endif

    buddy::puppies::dwarfs[dwarf_nr].set_fan(fan_nr, remapped_pwm);
    return true;
}

uint8_t CFanCtlPuppy::getPWM() const {
    return buddy::puppies::dwarfs[dwarf_nr].get_fan_pwm(fan_nr);
}

uint16_t CFanCtlPuppy::getActualRPM() const {
    return buddy::puppies::dwarfs[dwarf_nr].get_fan_rpm(fan_nr);
}

bool CFanCtlPuppy::getRPMIsOk() {
    return buddy::puppies::dwarfs[dwarf_nr].get_fan_rpm_ok(fan_nr);
}

CFanCtlCommon::FanState CFanCtlPuppy::getState() const {
    return static_cast<FanState>(buddy::puppies::dwarfs[dwarf_nr].get_fan_state(fan_nr));
}
