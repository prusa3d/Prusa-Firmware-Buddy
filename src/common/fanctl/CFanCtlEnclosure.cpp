#include <stdint.h>
#include <device/board.h>
#include "CFanCtlEnclosure.hpp"
#include <device/hal.h>
#include "cmsis_os.h"
#include <random.h>
#include "leds/side_strip.hpp"

CFanCtlEnclosure::CFanCtlEnclosure(const buddy::hw::InputPin &pin_tach,
    uint16_t min_rpm, uint16_t max_rpm)
    : min_rpm(min_rpm)
    , max_rpm(max_rpm)
    , tachometer(pin_tach) {
    buddy::hw::fanPowerSwitch.set();
    setPWM(0);
}

void CFanCtlEnclosure::set_actual_pwm(uint8_t pwm) {
    if (pwm != actual_pwm) {
        actual_pwm = pwm;
        leds::side_strip.SetEnclosureFanPwm(pwm);
    }
}

void CFanCtlEnclosure::tick() {
    bool edge = tachometer.tick();

    switch (state) {
    case idle:
        if (desired_pwm > 0) {
            state = starting;
            edges = 0;
            ticks = 0;
        } else {
            set_actual_pwm(0);
        }
        break;
    case starting:
        if (desired_pwm == 0) {
            state = idle;
        } else {
            ticks++;
            if (ticks > start_timeout) {
                state = error_starting;
            } else {
                set_actual_pwm(255);
                edges += edge ? 1 : 0;
                if (edges >= start_edges) {
                    state = rpm_stabilization;
                    ticks = 0;
                }
            }
        }
        break;
    case rpm_stabilization:
        if (desired_pwm == 0) {
            state = idle;
        } else {
            set_actual_pwm(desired_pwm);
            if (ticks < rpm_delay) {
                ticks++;
            } else {
                state = running;
            }
        }
        break;
    case running:
        if (desired_pwm == 0) {
            state = idle;
        } else {
            set_actual_pwm(desired_pwm);
            if (!getRPMIsOk()) {
                state = error_running;
            }
        }
        break;
    default: // error state
        if (desired_pwm == 0) {
            state = idle;
        } else {
            set_actual_pwm(desired_pwm);
            if (getRPMIsOk()) {
                state = running;
            }
        }
        break;
    }
}

bool CFanCtlEnclosure::Tachometer::tick() {
    bool current_level = static_cast<bool>(pin.read());
    bool edge = current_level ^ previous_level;
    edges += edge ? 1 : 0;
    previous_level = current_level;
    if (++ticks >= measurement_ticks_interval) {
        rpm = edges / 4 * 60;
        edges = 0;
        ticks = 0;
    }
    return edge;
}
