#include "CFanCtl3WireDynamic.hpp"
#include <fanctl/dynamic_pwm.hpp>
#include <option/has_love_board.h>
#include <hw_configuration.hpp>

CFanCtl3WireDynamic::CFanCtl3WireDynamic(
    const buddy::hw::OutputPin &pinOut,
    const buddy::hw::InputPin &pinTach,
    uint8_t minPWM,
    uint8_t maxPWM,
    uint16_t minRPM,
    uint16_t maxRPM,
    uint8_t thrPWM,
    is_autofan_t autofan,
    skip_tacho_t skip_tacho,
    uint8_t min_pwm_to_measure_rpm)
    : CFanCtl3Wire(
        [&pinOut](bool value) { pinOut.writeb(value); },
        [&pinTach]() { return pinTach.readb(); },
        {
            .min_pwm = minPWM,
            .max_pwm = maxPWM,
            .min_rpm = minRPM,
            .max_rpm = maxRPM,
            .thr_pwm = thrPWM,
            .autofan = autofan,
            .skip_tacho = skip_tacho,
            .min_pwm_to_measure_rpm = min_pwm_to_measure_rpm,
            .has_inverted_pwm = buddy::hw::Configuration::Instance().has_inverted_fans(),
        }) {
    setPhaseShiftMode(CFanCtlPWM::PhaseShiftMode::none);
}

uint16_t CFanCtl3WireDynamic::apply_dynamic_period(uint16_t pwm) {
    if (pwm == 0) {
        return 0;
    }
    pwm = std::clamp<uint16_t>(pwm, 255 * 9 / 100 /* 9% min */, 255);
    const uint8_t period = fanctl::dynamic_pwm_period(static_cast<uint8_t>(pwm));
    m_pwm.set_max_PWM(period);
    m_pwm.set_min_PWM(period / 10);
    m_pwm.set_PhaseShiftThr(period * 4 / 10);
    return pwm;
}

bool CFanCtl3WireDynamic::set_pwm(uint16_t pwm) {
    if (selftest_mode) {
        return false;
    }
    m_PWMValue = scalePWM(apply_dynamic_period(pwm));
    return true;
}

// The self test must use the same PWM periods as a print. Otherwise it keeps the period of the
// last set_pwm() and does not test the slow periods that low print fan speeds use.
bool CFanCtl3WireDynamic::selftest_set_pwm(uint8_t pwm) {
    if (!selftest_mode) {
        return false;
    }
    m_PWMValue = scalePWM(apply_dynamic_period(pwm));
    return true;
}
