#include "cpu_fan_controller.hpp"
#include <option/has_cpu_fan.h>

static_assert(HAS_CPU_FAN());

#include <fanctl.hpp>

namespace {

// Marlin task only.
PWM255OrAuto target_pwm { pwm_auto };

} // namespace

namespace cpu_fan_controller {

static_assert(compute_pwm(temp_off, 0) == 0, "invalid PWM for temp_off");
static_assert(compute_pwm(temp_full, 0) == 255, "invalid PWM for temp_full");

void update(float temp_c) {
    if (target_pwm.has_value()) {
        // Reasserted every tick, so the override also takes the fan back after
        // the M1978 fan test, which suspends update() for its duration.
        Fans::cpu().set_pwm(target_pwm->value);
        return;
    }
    Fans::cpu().set_pwm(compute_pwm(temp_c, Fans::cpu().get_pwm()));
}

void set_target_pwm(PWM255OrAuto target) {
    target_pwm = target;
    if (target_pwm.has_value()) {
        Fans::cpu().set_pwm(target_pwm->value);
    }
}

} // namespace cpu_fan_controller
