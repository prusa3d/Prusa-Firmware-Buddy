#pragma once

#include <fanctl/CFanCtl3Wire.hpp>

class CFanCtl3WireDynamic : public CFanCtl3Wire {
public:
    CFanCtl3WireDynamic(
        const buddy::hw::OutputPin &pinOut,
        const buddy::hw::InputPin &pinTach,
        uint8_t minPWM,
        uint8_t maxPWM,
        uint16_t minRPM,
        uint16_t maxRPM,
        uint8_t thrPWM,
        is_autofan_t autofan,
        skip_tacho_t skip_tacho,
        uint8_t min_pwm_to_measure_rpm);

    bool set_pwm(uint16_t pwm) override;
    bool selftest_set_pwm(uint8_t pwm) override;

private:
    /// Clamps a non-zero PWM (0-255), sets the PWM period for it, and returns the PWM to scale.
    uint16_t apply_dynamic_period(uint16_t pwm);
};
