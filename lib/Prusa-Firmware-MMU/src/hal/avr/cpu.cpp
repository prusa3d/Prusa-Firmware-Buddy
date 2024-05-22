/// @file cpu.cpp
#include "../cpu.h"
#include <avr/interrupt.h>
#include "../watchdog.h"

#include "lufa_config.h"
#include "Descriptors.h"
#include "lufa/LUFA/Drivers/USB/USB.h"

#include "../usart.h"

namespace hal {
namespace cpu {

bool resetPending = false;

void Init() {
}

void Reset() {
    cli();
    watchdog::Enable(watchdog::configuration::compute(0)); //quickest watchdog reset
    for (;;)
        ; //endless loop while waiting for the watchdog to reset
}

void Step() {
    if (resetPending) {
        hal::usart::usart1.puts("resetPending\n");
        for (;;)
            ; //endless loop while waiting for the watchdog to reset
    }
}

} // namespace CPU
} // namespace hal
