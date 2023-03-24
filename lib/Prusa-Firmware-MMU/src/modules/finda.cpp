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
    auto tgtMs = mt::timebase.Millis() + config::findaDebounceMs + 1;
    Step(); // let FINDA settle down - we're gonna need its state for selector homing
    while (tgtMs < mt::timebase.Millis()) {
        mf::finda.Step();
    }
}

} // namespace finda
} // namespace modules
