#include "c_fan_ctl.hpp"

void CFanCtl::EnterSelftestMode() {
    selftest_active = true;
}

void CFanCtl::ExitSelftestMode() {
    selftest_active = false;

    // upon exit selftestt, either switch fan back to AUTO mode, or turn it off and let marlin turn it on if needed
    reset_fan();
}

void CFanCtl::reset_fan() {
    setPWM(is_autofan ? buddy::puppies::Dwarf::FAN_MODE_AUTO_PWM : 0);
}

bool CFanCtl::setPWM(uint16_t pwm) {
    if (selftest_active) {
        return false;
    }

    buddy::puppies::dwarfs[dwarf_nr].set_fan(fan_nr, pwm);
    return true;
}

bool CFanCtl::SelftestSetPWM(uint8_t pwm) {
    if (!selftest_active) {
        return false;
    }

    buddy::puppies::dwarfs[dwarf_nr].set_fan(fan_nr, pwm);
    return true;
}

uint8_t CFanCtl::getPWM() const {
    return buddy::puppies::dwarfs[dwarf_nr].RegisterGeneralStatus.value.fan[fan_nr].pwm;
}

uint16_t CFanCtl::getActualRPM() const {
    return buddy::puppies::dwarfs[dwarf_nr].RegisterGeneralStatus.value.fan[fan_nr].rpm;
}

bool CFanCtl::getRPMIsOk() {
    return buddy::puppies::dwarfs[dwarf_nr].RegisterGeneralStatus.value.fan[fan_nr].is_rpm_ok;
}

CFanCtl::FanState CFanCtl::getState() const {
    return static_cast<FanState>(buddy::puppies::dwarfs[dwarf_nr].RegisterGeneralStatus.value.fan[fan_nr].state);
}
