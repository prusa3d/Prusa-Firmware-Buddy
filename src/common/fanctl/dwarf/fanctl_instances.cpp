
#include <fanctl.hpp>
#include "hwio_pindef.h"
#include "bsod.h"
#include <fanctl/CFanCtl3Wire.hpp>
#include <CFanCtlCommonConsts.hpp>
#include <option/has_love_board.h>

static auto write_print_pwm = [](bool value) {
    buddy::hw::fanPrintPwm.writeb(value);
};

static auto read_print_tacho = []() {
    return buddy::hw::fanPrintTach.readb();
};

CFanCtlCommon &Fans::print(PhysicalToolIndex) {
    static CFanCtl3Wire instance = CFanCtl3Wire(
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
            .has_inverted_pwm = false,
        });

    static_assert(PhysicalToolIndex::count == 1);
    return instance;
};

static auto write_heat_pwm = [](bool value) {
    buddy::hw::fanHeatBreakPwm.writeb(value);
};

static auto read_heat_tacho = []() {
    return buddy::hw::fanHeatBreakTach.readb();
};

CFanCtlCommon &Fans::heat_break(PhysicalToolIndex) {
    static CFanCtl3Wire instance = CFanCtl3Wire(
        write_heat_pwm,
        read_heat_tacho,
        {
            .min_pwm = FANCTLHEATBREAK_PWM_MIN,
            .max_pwm = FANCTLHEATBREAK_PWM_MAX,
            .min_rpm = FANCTLHEATBREAK_RPM_MIN,
            .max_rpm = FANCTLHEATBREAK_RPM_MAX,
            .thr_pwm = FANCTLHEATBREAK_PWM_THR,
            .autofan = is_autofan_t::yes,
            .skip_tacho = skip_tacho_t::no,
            .min_pwm_to_measure_rpm = FANCTLHEATBREAK_MIN_PWM_TO_MEASURE_RPM,
            .has_inverted_pwm = false,
        });

    static_assert(PhysicalToolIndex::count == 1);
    return instance;
};

void Fans::tick() {
    Fans::print(PhysicalToolIndex::from_raw(0)).tick();
    Fans::heat_break(PhysicalToolIndex::from_raw(0)).tick();
}

void Fans::init_hw() {}
