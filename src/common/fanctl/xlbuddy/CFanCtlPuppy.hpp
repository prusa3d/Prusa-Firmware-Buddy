#pragma once

#include <stdint.h>
#include "CFanCtlCommon.hpp"

class CFanCtlPuppy final : public CFanCtlCommon {
public:
    CFanCtlPuppy(uint8_t dwarf_nr, uint8_t fan_nr, bool is_autofan, uint16_t max_rpm)
        : CFanCtlCommon(0, max_rpm)
        , dwarf_nr(dwarf_nr)
        , fan_nr(fan_nr)
        , is_autofan(is_autofan) {
        reset_fan();
    }

    virtual void enterSelftestMode() override;

    virtual void exitSelftestMode() override;

    void reset_fan();

    virtual bool setPWM(uint16_t pwm) override;

    virtual bool selftestSetPWM(uint8_t pwm) override;

    virtual uint8_t getPWM() const override;

    virtual uint16_t getActualRPM() const override;

    virtual bool getRPMIsOk() const override;

    virtual FanState getState() const override;

    // Not used
    virtual uint16_t getMinPWM() const override { return 0; }
    virtual bool getRPMMeasured() const override { return false; }
    virtual void tick() override {}

private:
    const uint8_t dwarf_nr;
    const uint8_t fan_nr;
    const bool is_autofan;
};
