
#include "safe_state.h"
#include "hwio.h"
#include "wiring_digital.h"
#include "hwio_pindef.h"
#include "gpio.h"
#include "config.h"
#include "appmain.hpp"
#include <device/board.h>
#include "printers.h"
#include "fanctl.hpp"

using namespace buddy::hw;

#if BOARD_IS_BUDDY || BOARD_IS_XBUDDY
//! @brief Put hardware into safe state
//!
//! Set fans to maximum, heaters to minimum and disable motors.
void hwio_safe_state(void) {
    // enable fans
    fanCtlPrint[0].safeState();
    fanCtlHeatBreak[0].safeState();

    // disable heaters
    gpio_init(MARLIN_PIN(HEAT0), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_init(MARLIN_PIN(BED_HEAT), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PIN(HEAT0), 0);
    gpio_set(MARLIN_PIN(BED_HEAT), 0);
    // disable motors

    xEnable.write(Pin::State::high);
    #if BOARD_IS_BUDDY
    yEnable.write(Pin::State::high);
    #endif
    zEnable.write(Pin::State::high);
    e0Enable.write(Pin::State::high);
}

#elif BOARD_IS_XLBUDDY
void hwio_safe_state(void) {
    // Disable axes
    xyEnable.write(Pin::State::high);
    zEnable.write(Pin::State::high);

    // Set power panic for modular bed
    modularBedReset.set();

    // Reset all dwarfs to enable fans
    // Keep splitter powered to keep fans on.
    // Avoid power panic to keep fans on.
    dwarf1Reset.set();
    dwarf2Reset.set();
    dwarf3Reset.set();
    dwarf4Reset.set();
    dwarf5Reset.set();
    dwarf6Reset.set();
    HAL_Delay(1);
    dwarf1Reset.reset();
    dwarf2Reset.reset();
    dwarf3Reset.reset();
    dwarf4Reset.reset();
    dwarf5Reset.reset();
    dwarf6Reset.reset();

    // Disable ESP
    espPower.reset();

    // Disable USBs
    fsUSBPwrEnable.set();
    hsUSBEnable.set();
}
#elif BOARD_IS_DWARF
void hwio_safe_state(void) {
    // heater OFF
    heat0.write(Pin::State::low);

    // motor off
    e0Enable.write(Pin::State::high);

    fanCtlPrint[0].safeState();
    fanCtlHeatBreak[0].safeState();
}
#endif
