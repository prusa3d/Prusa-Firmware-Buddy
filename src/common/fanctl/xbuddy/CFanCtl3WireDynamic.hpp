#pragma once

#include "CFanCtl3Wire.hpp"

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

    bool setPWM(uint16_t pwm) override;
};
