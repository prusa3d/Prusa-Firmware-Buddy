#include "fanctl.hpp"
#include "bsod.h"
#include "Marlin/src/inc/MarlinConfig.h" // HOTENDS
#include <array>

CFanCtl &Fans::print(size_t index) {
    static std::array<CFanCtl, HOTENDS> instances = {
        CFanCtl(0, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtl(1, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtl(2, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtl(3, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtl(4, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtl(5, 0, false, FANCTLPRINT_RPM_MAX),
    };

    if (index > 5) {
        bsod("Print fan %u does not exist", index);
    }
    return instances[index];
}
CFanCtl &Fans::heat_break(size_t index) {
    static std::array<CFanCtl, HOTENDS> instances = {
        CFanCtl(0, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtl(1, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtl(2, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtl(3, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtl(4, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtl(5, 1, true, FANCTLHEATBREAK_RPM_MAX),
    };

    if (index > 5) {
        bsod("Heat break fan %u does not exist", index);
    }
    return instances[index];
}

void Fans::tick() {
    record_fanctl_metrics();
}
