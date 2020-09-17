
#include "safe_state.h"
#include "hwio.h"
#include "wiring_digital.h"
#include "hwio_pindef.h"
#include "gpio.h"
#include "config.h"
#include "appmain.hpp"

//! @brief Put hardware into safe state
//!
//! Set fans to maximum, heaters to minimum and disable motors.
void hwio_safe_state(void) {
    // enable fans
#ifdef NEW_FANCTL
    fanctl0.safeState();
    fanctl1.safeState();
#else
    gpio_init(MARLIN_PORT_PIN_FAN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_init(MARLIN_PORT_PIN_FAN1, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PORT_PIN_FAN, 1);
    gpio_set(MARLIN_PORT_PIN_FAN1, 1);
#endif
    // disable heaters
    gpio_init(MARLIN_PORT_PIN_HEAT0, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_init(MARLIN_PORT_PIN_BED_HEAT, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(MARLIN_PORT_PIN_HEAT0, 0);
    gpio_set(MARLIN_PORT_PIN_BED_HEAT, 0);
    // disable motors
    xEnable.write(GPIO_PinState::GPIO_PIN_SET);
    yEnable.write(GPIO_PinState::GPIO_PIN_SET);
    zEnable.write(GPIO_PinState::GPIO_PIN_SET);
    e0Enable.write(GPIO_PinState::GPIO_PIN_SET);
}
