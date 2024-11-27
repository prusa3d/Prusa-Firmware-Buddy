
#include <fanctl.hpp>
#include "hwio_pindef.h"
#include "CFanCtl3Wire.hpp"
#include "CFanCtl3WireDynamic.hpp"

#if !PRINTER_IS_PRUSA_MK4() && !PRINTER_IS_PRUSA_COREONE()
    #error "Dynamic PWM is only for MK4/CUBE, fix your CMakeLists.txt!"
#endif

CFanCtlCommon &Fans::print(size_t index) {
    static auto instance = CFanCtl3WireDynamic(
        buddy::hw::fanPrintPwm,
        buddy::hw::fanTach,
        FANCTLPRINT_PWM_MIN, FANCTLPRINT_PWM_MAX,
        FANCTLPRINT_RPM_MIN, FANCTLPRINT_RPM_MAX,
        FANCTLPRINT_PWM_THR,
        is_autofan_t::no,
        skip_tacho_t::yes,
        FANCTLPRINT_MIN_PWM_TO_MEASURE_RPM);

    if (index) {
        bsod("Print fan %u does not exist", index);
    }
    return instance;
};

CFanCtlCommon &Fans::heat_break(size_t index) {
    static CFanCtl3Wire instance = CFanCtl3Wire(
        buddy::hw::fanHeatBreakPwm,
        buddy::hw::fanTach,
        FANCTLHEATBREAK_PWM_MIN, FANCTLHEATBREAK_PWM_MAX,
        FANCTLHEATBREAK_RPM_MIN, FANCTLHEATBREAK_RPM_MAX,
        FANCTLHEATBREAK_PWM_THR,
        is_autofan_t::yes,
        skip_tacho_t::no,
        FANCTLHEATBREAK_MIN_PWM_TO_MEASURE_RPM);

    if (index) {
        bsod("Heat break fan %u does not exist", index);
    }
    return instance;
};

void Fans::tick() {
    CFanCtl3Wire &heatbreak_fan = static_cast<CFanCtl3Wire &>(Fans::heat_break(0));
    CFanCtl3Wire &print_fan = static_cast<CFanCtl3Wire &>(Fans::print(0));

    if (heatbreak_fan.getSkipTacho() != skip_tacho_t::yes && heatbreak_fan.getRPMMeasured()) {
        buddy::hw::tachoSelectPrintFan.write(buddy::hw::Pin::State::high);
        print_fan.setSkipTacho(skip_tacho_t::no);
        heatbreak_fan.setSkipTacho(skip_tacho_t::yes);
    } else if (print_fan.getSkipTacho() != skip_tacho_t::yes && print_fan.getRPMMeasured()) {
        buddy::hw::tachoSelectPrintFan.write(buddy::hw::Pin::State::low);
        heatbreak_fan.setSkipTacho(skip_tacho_t::no);
        print_fan.setSkipTacho(skip_tacho_t::yes);
    }
    Fans::print(0).tick();
    Fans::heat_break(0).tick();
    record_fanctl_metrics();
}
