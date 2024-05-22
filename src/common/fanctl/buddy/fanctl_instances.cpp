
#include <fanctl.hpp>
#include "hwio_pindef.h"
#include "bsod.h"
#include "CFanCtl3Wire.hpp"

CFanCtlCommon &Fans::print(size_t index) {
    static CFanCtl3Wire instance = CFanCtl3Wire(
        buddy::hw::fanPrintPwm,
        buddy::hw::fanPrintTach,
        FANCTLPRINT_PWM_MIN, FANCTLPRINT_PWM_MAX,
        FANCTLPRINT_RPM_MIN, FANCTLPRINT_RPM_MAX,
        FANCTLPRINT_PWM_THR,
        is_autofan_t::no,
        skip_tacho_t::no);

    if (index) {
        bsod("Print fan %u does not exist", index);
    }
    return instance;
};

CFanCtlCommon &Fans::heat_break(size_t index) {
    static CFanCtl3Wire instance = CFanCtl3Wire(
        buddy::hw::fanHeatBreakPwm,
        buddy::hw::fanHeatBreakTach,
        FANCTLHEATBREAK_PWM_MIN, FANCTLHEATBREAK_PWM_MAX,
        FANCTLHEATBREAK_RPM_MIN, FANCTLHEATBREAK_RPM_MAX,
        FANCTLHEATBREAK_PWM_THR,
        is_autofan_t::yes,
        skip_tacho_t::no);

    if (index) {
        bsod("Heat break fan %u does not exist", index);
    }
    return instance;
};

void Fans::tick() {
    Fans::print(0).tick();
    Fans::heat_break(0).tick();
    record_fanctl_metrics();
}
