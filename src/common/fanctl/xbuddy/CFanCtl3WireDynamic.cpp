#include "CFanCtl3WireDynamic.hpp"

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
        pinOut,
        pinTach,
        minPWM,
        maxPWM,
        minRPM,
        maxRPM,
        thrPWM,
        autofan,
        skip_tacho,
        min_pwm_to_measure_rpm) {
    setPhaseShiftMode(CFanCtlPWM::PhaseShiftMode::none);
}

bool CFanCtl3WireDynamic::setPWM(uint16_t pwm) {
    if (selftest_mode) {
        return false;
    }
    if (pwm) {
        pwm = std::clamp<uint16_t>(pwm, 255 * 9 / 100 /* 9% min */, 255);
        uint8_t val = 0;
        if (pwm < 50) {
            val = 250;
        } else if (pwm < 80) {
            val = 125;
        } else if (pwm < 120) {
            val = 50;
        } else {
            val = 40;
        }
        m_pwm.set_max_PWM(val);
        m_pwm.set_min_PWM(val / 10);
        m_pwm.set_PhaseShiftThr(val * 4 / 10);
    }
    m_PWMValue = scalePWM(pwm);
    return true;
}
