/**
 * @file c_fan_ctl.cpp
 */

#include <stdint.h>
#include <fanctl/CFanCtl3Wire.hpp>
#include <stdlib.h>
#include <random/random.h>
#include <random_sw/random_sw.h>
#include <algorithm>

//------------------------------------------------------------------------------
// CFanCtlPWM implementation

CFanCtlPWM::CFanCtlPWM(const WritePinFunc &pin_write_func, const PWMParams &pwm_params)
    : pin_write(pin_write_func)
    , min_value(pwm_params.pwm_min)
    , max_value(pwm_params.pwm_max)
    , pha_ena(false)
    , pwm(0)
    , cnt(0)
    , val(0)
    , pha(0)
    , pha_mode(random)
    , pha_thr(pwm_params.phase_shift_threshold)
    , pha_max(0)
    , pha_stp(0)
    , has_inverted_pwm(pwm_params.has_inverted_pwm) {}

int8_t CFanCtlPWM::tick() {
    int8_t pwm_on = cnt - pha; // calculate on time (number of ticks after 0-1 pwm transition)
    if (pwm_on >= val) {
        pwm_on -= max_value;
    }
    const bool o = (cnt >= pha) && (cnt < (pha + val));
    if (++cnt >= max_value) {
        cnt = 0;
        if (val != pwm) { // pwm changed
            val = pwm; // update cached value
            pha = 0; // reset phase
            pha_max = max_value - val; // calculate maximum phase
            if ((val > 1) && (val <= pha_thr)) {
                uint8_t steps = max_value / val; // calculate number of steps
                if (steps < 3) {
                    steps = 3; // limit steps >= 3
                }
                pha_stp = pha_max / steps; // calculate step - enable phase shifting
            } else {
                pha_stp = 0; // set step to zero - disable phase shifting
            }
        } else if (pha_stp) { // pha_stp != 0 means phase shifting enabled
            switch (pha_mode) {
            case none:
                pha = 0;
                break;
            case triangle:
                pha += pha_stp;
                if (pha >= pha_max) {
                    pha_stp = -pha_stp;
                    pha = pha_max;
                } else if (pha < 0) {
                    pha_stp = -pha_stp;
                    pha = 0;
                }
                break;
            case random:
                pha = static_cast<uint8_t>(pha_max * rand_f_from_u(rand_u_sw()));
                break;
            }
        }
    }
    pin_write(o ^ has_inverted_pwm);

    return pwm_on;
}

void CFanCtlPWM::set_PWM(uint8_t new_pwm) {
    pwm = (new_pwm > 0) ? std::clamp(new_pwm, min_value, max_value) : 0;
}

void CFanCtlPWM::safeState() {
    set_PWM(max_value);
    pin_write(!has_inverted_pwm);
}

//------------------------------------------------------------------------------
// CFanCtlTach implementation

CFanCtlTach::CFanCtlTach(const ReadPinFunc &read_pin_func)
    : pin_read(read_pin_func) {
    tick_count = 0;
    ticks_per_second = 1000;
    edges = 0;
    pwm_sum = 0;
    rpm = 0;
    input_state = false;
    m_value_ready = false;
}

bool CFanCtlTach::tick(int8_t pwm_on) {
    bool tach = pin_read(); // sample tach input pin
    bool edge = ((tach ^ input_state) && (pwm_on >= 2)); // detect edge inside pwm pulse, ignore first two sub-periods after 0-1 pwm transition
    if (edge) {
        edges++;
    }
    input_state = tach; // store current tach input state
    if (++tick_count >= ticks_per_second) {
        if (pwm_sum) {
            edges = (edges * ticks_per_second) / pwm_sum; // add lost edges
        }
        rpm = (rpm + (45 * edges)) >> 2; // calculate and filter rpm original formula
                                         // rpm = (rpm + 3 * ((60 * edges) >> 2)) >> 2;
                                         // take original rpm add 3 times new rpm - new rpm= 60*freq;
                                         // freq=edges/2/2; edges= revolutions per second *2 *2 (2 poles motor and two edges per revolution)
        edges = 0; // reset edge counter
        tick_count = 0; // reset tick counter
        pwm_sum = 0; // reset pwm_sum
        m_value_ready = true; // set value ready = measure done
    } else if (pwm_on >= 0) {
        pwm_sum++; // inc pwm sum if pwm enabled
    }
    return edge;
}

//------------------------------------------------------------------------------
// CFanCtl3Wire implementation

CFanCtl3Wire::CFanCtl3Wire(const CFanCtlPWM::WritePinFunc &write_pin_func, const CFanCtlTach::ReadPinFunc &read_pin_func,
    const FanParams &fan_params)
    : CFanCtlCommon(fan_params.min_rpm, fan_params.max_rpm)
    , m_State(idle)
    , m_PWMValue(0)
    , min_pwm_to_measure_rpm(fan_params.min_pwm_to_measure_rpm)
    , is_autofan(fan_params.autofan)
    , m_pwm(write_pin_func, {
                                .pwm_min = fan_params.min_pwm,
                                .pwm_max = fan_params.max_pwm,
                                .phase_shift_threshold = fan_params.thr_pwm,
                                .has_inverted_pwm = fan_params.has_inverted_pwm,
                            })
    , m_tach(read_pin_func)
    , m_skip_tacho(fan_params.skip_tacho) {
}

void CFanCtl3Wire::tick() {
    if (!autocontrol_enabled) {
        return;
    }
    // PWM control
    int8_t pwm_on = m_pwm.tick();
    // RPM measurement
    bool edge = false;

    if (m_skip_tacho != skip_tacho_t::yes) {
        edge = m_tach.tick(pwm_on);
    }

    switch (m_State) {
    case idle:
        if (m_PWMValue > 0) {
            m_State = starting;
            m_Edges = 0;
            m_Ticks = 0;
        } else {
            m_pwm.set_PWM(0);
        }
        break;
    case starting:
        if (m_PWMValue == 0) {
            m_State = idle;
        } else {
            m_Ticks++;
            if (m_Ticks > FANCTL_START_TIMEOUT && m_skip_tacho != skip_tacho_t::yes) {
                m_State = error_starting;
            } else {
                m_pwm.set_PWM(m_pwm.get_max_PWM());
                if (edge) {
                    m_Edges++;
                }
                if (m_Edges >= FANCTL_START_EDGES) {
                    m_State = rpm_stabilization;
                    m_Ticks = 0;
                }
            }
        }
        break;
    case rpm_stabilization:
        if (m_PWMValue == 0) {
            m_State = idle;
        } else {
            m_pwm.set_PWM(m_PWMValue);
            if (m_Ticks < FANCTL_RPM_DELAY) {
                m_Ticks++;
            } else {
                m_State = running;
            }
        }
        break;
    case running:
        if (m_PWMValue == 0) {
            m_State = idle;
        } else {
            m_pwm.set_PWM(m_PWMValue);
            if (!get_rpm_is_ok() && m_skip_tacho != skip_tacho_t::yes) {
                m_State = error_running;
            }
        }
        break;
    default: // error state
        if (m_PWMValue == 0) {
            m_State = idle;
        } else {
            m_pwm.set_PWM(m_PWMValue);
            if (get_rpm_is_ok()) {
                m_State = running;
            }
        }
        break;
    }
}

uint16_t CFanCtl3Wire::scalePWM(uint16_t pwm) const {
    return pwm * m_pwm.get_max_PWM() / 255;
}

uint16_t CFanCtl3Wire::unscalePWM(uint16_t pwm) const {
    return pwm * 255 / m_pwm.get_max_PWM();
}

bool CFanCtl3Wire::set_pwm(uint16_t pwm) {
    if (selftest_mode) {
        return false;
    }
    m_PWMValue = scalePWM(pwm);
    return true;
}

bool CFanCtl3Wire::selftest_set_pwm(uint8_t pwm) {
    if (!selftest_mode) {
        return false;
    }
    m_PWMValue = scalePWM(pwm);
    return true;
}

bool CFanCtl3Wire::setPhaseShiftMode(CFanCtlPWM::PhaseShiftMode psm) {
    if (selftest_mode) {
        return false;
    }
    m_pwm.set_PhaseShiftMode(psm);
    return true;
}

void CFanCtl3Wire::safe_state() {
    set_pwm(m_pwm.get_max_PWM());
    m_pwm.safeState();
    selftest_mode = false;
}

bool CFanCtl3Wire::get_rpm_is_ok() const {
    if (m_PWMValue > min_pwm_to_measure_rpm && (get_actual_rpm() < min_rpm)) {
        return false;
    }
    return true;
}

void CFanCtl3Wire::enter_selftest_mode() {
    if (selftest_mode) {
        return;
    }
    selftest_mode = true;
    selftest_initial_pwm = get_pwm();
}

void CFanCtl3Wire::exit_selftest_mode() {
    if (!selftest_mode) {
        return;
    }
    selftest_mode = false;

    uint8_t pwm_to_restore;
    if (isAutoFan()) {
        // if this is autofan, turn fan off and let marlin turn it back on in case it is needed
        pwm_to_restore = 0;
    } else {
        pwm_to_restore = selftest_initial_pwm.load();
    }

    set_pwm(pwm_to_restore);
}
