/// @file timebase.cpp
#include "timebase.h"
#include "../hal/timers.h"
#include <avr/interrupt.h>

namespace modules {
namespace time {

Timebase timebase;

void Timebase::Init() {
    hal::timers::Tim8bit_CTC_config tim8ctc_conf = {
        .cs = 3, // ck/64
        .ocra = 250 - 1,
    };
    hal::timers::Configure_CTC8(0, TIMER0, &tim8ctc_conf);
}

void Timebase::Isr() {
    ms++;
}

bool Timebase::Elapsed(uint16_t start, uint16_t timeout) const {
    uint16_t ms_from_start = Millis(); // beware the uint16_t!
    ms_from_start -= start;
    return ms_from_start > timeout;
}

uint16_t Timebase::Millis() const {
    return ms;
}

} // namespace time
} // namespace modules

ISR(TIMER0_COMPA_vect) {
    modules::time::timebase.Isr();
}
