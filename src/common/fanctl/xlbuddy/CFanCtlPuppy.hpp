#pragma once

#include <stdint.h>
#include <fanctl/CFanCtlCommon.hpp>
#include <option/has_dwarf.h>
#include <option/has_indx.h>

#if HAS_DWARF()
    #include <puppies/Dwarf.hpp>
#elif HAS_INDX()
    #include <puppies/INDX.hpp>
#endif

class CFanCtlPuppy final : public CFanCtlCommon {
public:
    CFanCtlPuppy([[maybe_unused]] PhysicalToolIndex dwarf_nr, uint8_t fan_nr, bool is_autofan, uint16_t max_rpm)
        : CFanCtlCommon(0, max_rpm)
#if HAS_DWARF()
        , tool(buddy::puppies::dwarfs[dwarf_nr])
#elif HAS_INDX()
        , tool(buddy::puppies::indx)
#endif
        , fan_nr(fan_nr)
        , is_autofan(is_autofan) {
        reset_fan();
    }

    virtual void enter_selftest_mode() override;

    virtual void exit_selftest_mode() override;

    void reset_fan();

    virtual bool set_pwm(uint16_t pwm) override;

    virtual bool selftest_set_pwm(uint8_t pwm) override;

    virtual uint8_t get_pwm() const override;

    virtual uint16_t get_actual_rpm() const override;

    virtual bool get_rpm_is_ok() const override;

    virtual FanState get_state() const override;

    void safe_state() override {}; // Don't do anything, Dwarf gets completely shut down during hwio_safe_state

    // Not used
    virtual uint16_t get_min_pwm() const override { return 0; }
    virtual bool get_rpm_measured() const override { return false; }
    virtual void tick() override {}

private:
#if HAS_DWARF()
    buddy::puppies::Dwarf &tool;
#elif HAS_INDX()
    buddy::puppies::Indx &tool;
#endif
    const uint8_t fan_nr;
    const bool is_autofan;
};
