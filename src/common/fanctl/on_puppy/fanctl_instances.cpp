#include "fanctl.hpp"

std::array<CFanCtlOnPuppy, HOTENDS> fanCtlPrint = {
    CFanCtlOnPuppy(0, 0, false, FANCTLPRINT_RPM_MAX),
    CFanCtlOnPuppy(1, 0, false, FANCTLPRINT_RPM_MAX),
    CFanCtlOnPuppy(2, 0, false, FANCTLPRINT_RPM_MAX),
    CFanCtlOnPuppy(3, 0, false, FANCTLPRINT_RPM_MAX),
    CFanCtlOnPuppy(4, 0, false, FANCTLPRINT_RPM_MAX),
    CFanCtlOnPuppy(5, 0, false, FANCTLPRINT_RPM_MAX),
};
std::array<CFanCtlOnPuppy, HOTENDS> fanCtlHeatBreak = {
    CFanCtlOnPuppy(0, 1, true, FANCTLHEATBREAK_RPM_MAX),
    CFanCtlOnPuppy(1, 1, true, FANCTLHEATBREAK_RPM_MAX),
    CFanCtlOnPuppy(2, 1, true, FANCTLHEATBREAK_RPM_MAX),
    CFanCtlOnPuppy(3, 1, true, FANCTLHEATBREAK_RPM_MAX),
    CFanCtlOnPuppy(4, 1, true, FANCTLHEATBREAK_RPM_MAX),
    CFanCtlOnPuppy(5, 1, true, FANCTLHEATBREAK_RPM_MAX),
};

void fanctl_tick() {
    record_fanctl_metrics();
}
