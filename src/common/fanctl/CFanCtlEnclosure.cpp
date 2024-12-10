#include <stdint.h>
#include <device/board.h>
#include "CFanCtlEnclosure.hpp"
#include <device/hal.h>
#include "cmsis_os.h"
#include <random.h>
#include "leds/side_strip.hpp"

CFanCtlEnclosure::CFanCtlEnclosure(const buddy::hw::InputPin &pin_tach,
    uint16_t min_rpm, uint16_t max_rpm)
    : CFanCtlCommon(min_rpm, max_rpm)
    , tachometer(pin_tach) {
    buddy::hw::fanPowerSwitch.set();
    setPWM(0);
}

void CFanCtlEnclosure::tick() {
    bool edge = tachometer.tick();

    if (desired_pwm == 0) {
        state = idle;
    }
    switch (state) {
    case idle:
        if (desired_pwm > 0) {
            state = starting;
            edges = 0;
            ticks = 0;
        } else {
            leds::side_strip.SetEnclosureFanPwm(0);
        }
        break;
    case starting:
        ticks++;
        if (ticks > start_timeout) {
            state = error_starting;
        } else {
            leds::side_strip.SetEnclosureFanPwm(255);
            edges += edge ? 1 : 0;
            if (edges >= start_edges) {
                state = rpm_stabilization;
                ticks = 0;
            }
        }
        break;
    case rpm_stabilization:
        leds::side_strip.SetEnclosureFanPwm(desired_pwm);
        if (ticks < rpm_delay) {
            ticks++;
        } else {
            state = running;
        }
        break;
    case running:
        leds::side_strip.SetEnclosureFanPwm(desired_pwm);
        if (!getRPMIsOk()) {
            state = error_running;
        }
        break;
    default: // error state
        leds::side_strip.SetEnclosureFanPwm(desired_pwm);
        if (getRPMIsOk()) {
            state = running;
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

bool CFanCtlEnclosure::setPWM(uint16_t pwm) {
    if (selftest_mode) {
        selftest_initial_pwm = pwm > 255 ? 255 : static_cast<uint8_t>(pwm);
    } else {
        desired_pwm = pwm > 255 ? 255 : static_cast<uint8_t>(pwm);
    }
    return true;
}

void CFanCtlEnclosure::enterSelftestMode() {
    if (selftest_mode) {
        return;
    }
    selftest_mode = true;
    selftest_initial_pwm = getPWM();
}

void CFanCtlEnclosure::exitSelftestMode() {
    if (!selftest_mode) {
        return;
    }
    selftest_mode = false;
    setPWM(selftest_initial_pwm.load());
    selftest_initial_pwm = 0;
}

bool CFanCtlEnclosure::selftestSetPWM(uint8_t pwm) {
    if (!selftest_mode) {
        return false;
    }
    desired_pwm = pwm; // Set PWM directly without setPWM function
    return true;
}
