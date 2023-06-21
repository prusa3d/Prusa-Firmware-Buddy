
#include <fanctl.hpp>
#include "hwio_pindef.h"

CFanCtl &Fans::print(size_t index) {
    static CFanCtl instance = CFanCtl(
        buddy::hw::fanPrintPwm,
        buddy::hw::fanTach,
        FANCTLPRINT_PWM_MIN, FANCTLPRINT_PWM_MAX,
        FANCTLPRINT_RPM_MIN, FANCTLPRINT_RPM_MAX,
        FANCTLPRINT_PWM_THR,
        is_autofan_t::no,
        skip_tacho_t::yes);

    if (index) {
        bsod("Print fan %u does not exist", index);
    }
    return instance;
};

CFanCtl &Fans::heat_break(size_t index) {
    static CFanCtl instance = CFanCtl(
        buddy::hw::fanHeatBreakPwm,
        buddy::hw::fanTach,
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
    if (Fans::heat_break(0).getSkipTacho() != skip_tacho_t::yes && Fans::heat_break(0).getRPMMeasured()) {
        buddy::hw::tachoSelectPrintFan.write(buddy::hw::Pin::State::high);
        Fans::print(0).setSkipTacho(skip_tacho_t::no);
        Fans::heat_break(0).setSkipTacho(skip_tacho_t::yes);
    } else if (Fans::print(0).getSkipTacho() != skip_tacho_t::yes && Fans::print(0).getRPMMeasured()) {
        buddy::hw::tachoSelectPrintFan.write(buddy::hw::Pin::State::low);
        Fans::heat_break(0).setSkipTacho(skip_tacho_t::no);
        Fans::print(0).setSkipTacho(skip_tacho_t::yes);
    }
    Fans::print(0).tick();
    Fans::heat_break(0).tick();
    record_fanctl_metrics();
}
