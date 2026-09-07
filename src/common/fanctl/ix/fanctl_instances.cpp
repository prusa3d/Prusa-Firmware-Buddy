
#include <fanctl.hpp>
#include "fan_ctl_ix_turbine.hpp"
#include "hwio_pindef.h"
#include <fanctl/CFanCtl3Wire.hpp>
#include <CFanCtlCommonConsts.hpp>
#include <option/has_love_board.h>
#include <bsod/bsod.h>

static auto write_print_pwm = [](bool value) {
    buddy::hw::fanPrintPwm.writeb(value);
};

static auto read_print_tacho = []() {
    return buddy::hw::fanTach.readb();
};

CFanCtlCommon &Fans::print(PhysicalToolIndex) {
    static auto instance = CFanCtl3Wire(
        write_print_pwm,
        read_print_tacho,
        {
            .min_pwm = FANCTLPRINT_PWM_MIN,
            .max_pwm = FANCTLPRINT_PWM_MAX,
            .min_rpm = FANCTLPRINT_RPM_MIN,
            .max_rpm = FANCTLPRINT_RPM_MAX,
            .thr_pwm = FANCTLPRINT_PWM_THR,
            .autofan = is_autofan_t::no,
            .skip_tacho = skip_tacho_t::no,
            .min_pwm_to_measure_rpm = FANCTLPRINT_MIN_PWM_TO_MEASURE_RPM,
            .has_inverted_pwm = buddy::hw::Configuration::Instance().has_inverted_fans(),

        });

    static_assert(PhysicalToolIndex::count == 1);
    return instance;
};

CFanCtlCommon &Fans::heat_break(PhysicalToolIndex) {
    static FanCtlIxTurbine instance;

    static_assert(PhysicalToolIndex::count == 1);
    return instance;
};

void Fans::tick() {
    Fans::print(PhysicalToolIndex::from_raw(0)).tick();
}

void Fans::init_hw() {}
