#include "stub_timebase.h"
#include "../../../../src/modules/timebase.h"

namespace modules {
namespace time {

Timebase timebase;

uint16_t millis = 0;

void Timebase::Init() {}

void Timebase::Isr() {}

uint16_t Timebase::Millis() const {
    return millis;
}

void ReinitTimebase(uint16_t ms /* = 0 */) {
    millis = ms;
}

void IncMillis(uint16_t inc /* = 1*/) {
    millis += inc;
}

bool Timebase::Elapsed(uint16_t start, uint16_t timeout) const {
    uint16_t ms_from_start = Millis(); // beware the uint16_t!
    ms_from_start -= start;
    return ms_from_start > timeout;
}

} // namespace time
} // namespace modules
