
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
#include <option/has_puppies.h>
#include <option/has_dwarf.h>
#include "CFanCtl3Wire.hpp"

using namespace buddy::hw;

//! @brief Put hardware into safe state
//!
//! Set fans to maximum, heaters to minimum and disable motors.
void hwio_safe_state(void) {
#if BOARD_IS_BUDDY() || BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
    // enable fans, disable hotends
    #if HAS_DWARF()
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
    #else
    static_cast<CFanCtl3Wire &>(Fans::print(0)).safeState();
    static_cast<CFanCtl3Wire &>(Fans::heat_break(0)).safeState();

    // disable hotend
    gpio_init(MARLIN_PIN(HEAT0), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PIN(HEAT0), 0);
    #endif
    // Disable heated bed
    #if HAS_MODULARBED()
    // Set power panic for modular bed
    modularBedReset.set();
    #else
    // disable heatbed
    gpio_init(MARLIN_PIN(BED_HEAT), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PIN(BED_HEAT), 0);
    #endif

    // disable motors
    #if BOARD_IS_XLBUDDY()
    xyEnable.write(Pin::State::high);
    #else
    xEnable.write(Pin::State::high);
        #if BOARD_IS_BUDDY()
    yEnable.write(Pin::State::high);
        #endif
    e0Enable.write(Pin::State::high);
    #endif
    zEnable.write(Pin::State::high);

    #if BOARD_IS_XLBUDDY()
    // Disable ESP
    espPower.reset();

    // Disable USBs
    fsUSBPwrEnable.set();
    hsUSBEnable.set();
    #endif
#elif BOARD_IS_DWARF()
    // heater OFF
    heat0.write(Pin::State::low);

    // motor off
    e0Enable.write(Pin::State::high);

    static_cast<CFanCtl3Wire &>(Fans::print(0)).safeState();
    static_cast<CFanCtl3Wire &>(Fans::heat_break(0)).safeState();
#endif
}

void hwio_low_power_state(void) {
#if BOARD_IS_XLBUDDY()
    // Disable ESP
    espPower.reset();

    // Disable USBs
    fsUSBPwrEnable.set();
    hsUSBEnable.set();

    // disable 5V regulator on splitter
    splitter5vEnable.reset();
#endif
}

void buddy_disable_heaters(void) {
    // this function is called before breakpoint, and has to be callable from ISR.
    // Do not do any mutexes etc. Only simple pin
#if BOARD_IS_BUDDY() || BOARD_IS_XBUDDY()
    gpio_init(MARLIN_PIN(HEAT0), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PIN(HEAT0), 0);

    gpio_init(MARLIN_PIN(BED_HEAT), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PIN(BED_HEAT), 0);
#endif
}
