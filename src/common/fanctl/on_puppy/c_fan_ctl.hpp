#pragma once

#include <stdint.h>
#include "CFanCtlCommon.hpp"
#include "puppies/Dwarf.hpp"

class CFanCtl : public CfanCtlCommon {
public:
    CFanCtl(uint8_t dwarf_nr, uint8_t fan_nr, bool is_autofan, uint16_t max_rpm)
        : dwarf_nr(dwarf_nr)
        , fan_nr(fan_nr)
        , is_autofan(is_autofan)
        , max_rpm(max_rpm)
        , selftest_active(false) {
        reset_fan();
    }

    void EnterSelftestMode();

    void ExitSelftestMode();

    void reset_fan();

    bool setPWM(uint16_t pwm);

    bool SelftestSetPWM(uint8_t pwm);

    uint8_t getPWM() const;

    uint16_t getActualRPM() const;

    bool getRPMIsOk();

    FanState getState() const;

    inline uint8_t getMaxPWM() const {
        return 255;
    }

    inline uint16_t getMaxRPM() const {
        return max_rpm;
    }

private:
    const uint8_t dwarf_nr;
    const uint8_t fan_nr;
    const bool is_autofan;
    const uint16_t max_rpm;

    bool selftest_active;
};
