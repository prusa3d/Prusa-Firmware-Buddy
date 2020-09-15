
#include "safe_state.h"
#include "hwio.h"
#include "wiring_digital.h"
#include "hwio_pindef.h"
#include "gpio.h"
#include "config.h"
#include "fanctl.h"

//! @brief Put hardware into safe state
//!
//! Set fans to maximum, heaters to minimum and disable motors.
void hwio_safe_state(void) {
    // enable fans
    gpio_set(PIN_FAN, 1);
    gpio_set(PIN_FAN1, 1);
    // disable heaters
    gpio_init(PIN_HEATER_0, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_init(PIN_HEATER_BED, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    gpio_set(PIN_HEATER_0, 0);
    gpio_set(PIN_HEATER_BED, 0);
    // disable motors
    gpio_set(PIN_X_ENABLE, 1);
    gpio_set(PIN_Y_ENABLE, 1);
    gpio_set(PIN_Z_ENABLE, 1);
    gpio_set(PIN_E_ENABLE, 1);
    // set 100% pwm to fanctl to prevent overwriting from interupt
    // this is not necessary - interrupts are disabled, but for sure
    // memory can be damaged, so we will do this after hardware setup
    fanctl_set_pwm(0, FANCTL0_PWM_MAX);
    fanctl_set_pwm(1, FANCTL1_PWM_MAX);
}
