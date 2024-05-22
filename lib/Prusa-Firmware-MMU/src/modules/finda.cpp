/// @file finda.cpp
#include "finda.h"
#include "timebase.h"
#include "../hal/gpio.h"
#include "../pins.h"

namespace modules {
namespace finda {

FINDA finda;

void FINDA::Step() {
    debounce::Debouncer::Step(mt::timebase.Millis(), hal::gpio::ReadPin(FINDA_PIN) == hal::gpio::Level::high);
}

void FINDA::BlockingInit() {
    uint16_t start = mt::timebase.Millis();
    // let FINDA settle down - we're gonna need its state for selector homing
    while (!mt::timebase.Elapsed(start, config::findaDebounceMs + 1)) {
        Step();
    }
}

} // namespace finda
} // namespace modules
