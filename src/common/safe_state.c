
#include "safe_state.h"
#include "hwio_a3ides.h"

//! @brief Put hardware into safe state
//!
//! Set fans to maximum, heaters to minimum and disable motors.
void hwio_safe_state(void) {

    for (int i = 0; i < hwio_fan_get_cnt(); ++i)
        hwio_fan_set_pwm(i, 255);

    for (int i = 0; i < hwio_heater_get_cnt(); ++i)
        hwio_heater_set_pwm(i, 0);

    //enable 1 means disable :(
    hwio_do_set_val(_DO_Z_ENABLE, 1);
    hwio_do_set_val(_DO_X_ENABLE, 1);
    hwio_do_set_val(_DO_E_ENABLE, 1);
    hwio_do_set_val(_DO_Y_ENABLE, 1);
}
