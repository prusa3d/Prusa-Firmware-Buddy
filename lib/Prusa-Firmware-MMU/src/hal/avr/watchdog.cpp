/// @file watchdog.cpp
#include "../watchdog.h"
#include <avr/wdt.h>

namespace hal {
namespace watchdog {

void Enable(const configuration &config) {
    wdt_enable(config.prescalerBits);
}

void Disable() {
    wdt_disable();
}

void Reset() {
    wdt_reset();
}

} // namespace watchdog
} // namespace hal
