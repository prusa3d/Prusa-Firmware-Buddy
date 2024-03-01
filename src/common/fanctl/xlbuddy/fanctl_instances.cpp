#include "fanctl.hpp"
#include "bsod.h"
#include "Marlin/src/inc/MarlinConfig.h" // HOTENDS
#include <array>

CFanCtlPuppy &Fans::print(size_t index) {
    static std::array<CFanCtlPuppy, HOTENDS> instances = {
        CFanCtlPuppy(0, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(1, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(2, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(3, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(4, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(5, 0, false, FANCTLPRINT_RPM_MAX),
    };

    if (index > 5) {
        bsod("Print fan %u does not exist", index);
    }
    return instances[index];
}
CFanCtlPuppy &Fans::heat_break(size_t index) {
    static std::array<CFanCtlPuppy, HOTENDS> instances = {
        CFanCtlPuppy(0, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(1, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(2, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(3, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(4, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(5, 1, true, FANCTLHEATBREAK_RPM_MAX),
    };

    if (index > 5) {
        bsod("Heat break fan %u does not exist", index);
    }
    return instances[index];
}

CFanCtlEnclosure &Fans::enclosure() {
    static auto instance = CFanCtlEnclosure(
        buddy::hw::fan1_tach0,
        FANCTLENCLOSURE_RPM_MIN, FANCTLENCLOSURE_RPM_MAX);

    return instance;
};

void Fans::tick() {
    Fans::enclosure().tick();
    record_fanctl_metrics();
}
