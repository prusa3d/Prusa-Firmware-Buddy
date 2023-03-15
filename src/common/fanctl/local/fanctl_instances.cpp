
#include <fanctl.hpp>
#include "hwio_pindef.h"
#include "config_buddy_2209_02.h"

std::array<CFanCtlLocal, 1> fanCtlPrint = {
    CFanCtlLocal(
        buddy::hw::fanPrintPwm,
#if (BOARD_IS_BUDDY || BOARD_IS_DWARF)
        buddy::hw::fanPrintTach,
#else
        buddy::hw::fanTach,
#endif
        FANCTLPRINT_PWM_MIN, FANCTLPRINT_PWM_MAX,
        FANCTLPRINT_RPM_MIN, FANCTLPRINT_RPM_MAX,
        FANCTLPRINT_PWM_THR,
        is_autofan_t::no,
#if (BOARD_IS_BUDDY || BOARD_IS_DWARF)
        skip_tacho_t::no
#else
        skip_tacho_t::yes
#endif
        )
};
std::array<CFanCtlLocal, 1> fanCtlHeatBreak = { CFanCtlLocal(
    buddy::hw::fanHeatBreakPwm,
#if (BOARD_IS_BUDDY || BOARD_IS_DWARF)
    buddy::hw::fanHeatBreakTach,
#else
    buddy::hw::fanTach,
#endif
    FANCTLHEATBREAK_PWM_MIN, FANCTLHEATBREAK_PWM_MAX,
    FANCTLHEATBREAK_RPM_MIN, FANCTLHEATBREAK_RPM_MAX,
    FANCTLHEATBREAK_PWM_THR,
    is_autofan_t::yes,
    skip_tacho_t::no) };

void fanctl_tick() {
#if (BOARD_IS_BUDDY || BOARD_IS_DWARF)
#else
    if (fanCtlHeatBreak[0].getSkipTacho() != skip_tacho_t::yes && fanCtlHeatBreak[0].getRPMMeasured()) {
        buddy::hw::tachoSelectPrintFan.write(buddy::hw::Pin::State::high);
        fanCtlPrint[0].setSkipTacho(skip_tacho_t::no);
        fanCtlHeatBreak[0].setSkipTacho(skip_tacho_t::yes);
    } else if (fanCtlPrint[0].getSkipTacho() != skip_tacho_t::yes && fanCtlPrint[0].getRPMMeasured()) {
        buddy::hw::tachoSelectPrintFan.write(buddy::hw::Pin::State::low);
        fanCtlHeatBreak[0].setSkipTacho(skip_tacho_t::no);
        fanCtlPrint[0].setSkipTacho(skip_tacho_t::yes);
    }
#endif
    fanCtlPrint[0].tick();
    fanCtlHeatBreak[0].tick();
#if !BOARD_IS_DWARF
    record_fanctl_metrics();
#endif
}
