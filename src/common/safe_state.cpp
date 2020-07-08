
#include "safe_state.h"
#include "hwio.h"
#include "wiring_digital.h"
#include "hwio_pindef.h"
#include "gpio.h"

//! @brief Put hardware into safe state
//!
//! Set fans to maximum, heaters to minimum and disable motors.
void hwio_safe_state(void) {

    for (int i = 0; i < hwio_fan_get_cnt(); ++i)
        hwio_fan_set_pwm(i, 255);

    for (int i = 0; i < hwio_heater_get_cnt(); ++i)
        hwio_heater_set_pwm(i, 0);

    //enable 1 means disable :(
    xEnable.write(GPIO_PinState::GPIO_PIN_SET);
    yEnable.write(GPIO_PinState::GPIO_PIN_SET);
    zEnable.write(GPIO_PinState::GPIO_PIN_SET);
    e0Enable.write(GPIO_PinState::GPIO_PIN_SET);
}
