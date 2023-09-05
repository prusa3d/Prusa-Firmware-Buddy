#include <mmu2_power.h>
#include "stub_interfaces.h"

namespace MMU2 {

void power_on() {
    mockLog.Record(__FUNCTION__);
}

void power_off() {
    mockLog.Record(__FUNCTION__);
}

void reset() {
    mockLog.Record(__FUNCTION__);
}

} // namespace MMU2
