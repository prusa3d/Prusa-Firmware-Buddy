// fanctl.cpp

#include "config_buddy_2209_02.h"
#ifdef NEW_FANCTL

    #include "fanctl.h"
    #include "stm32f4xx_hal.h"
    #include "cmsis_os.h"
    #include "gpio.h"
    #include <stdlib.h>
    #include <device/board.h>

using namespace buddy::hw;

// global variables for C wrapper
static uint8_t CFanCtl_count = 0;                  // number of instances
static CFanCtl *CFanCtl_instance[FANCTL_MAX_FANS]; // array of pointers to instances

//------------------------------------------------------------------------------
// CFanCtlPWM implementation

CFanCtlPWM::CFanCtlPWM(const OutputPin &pinOut, uint8_t pwm_min, uint8_t pwm_max, uint8_t phase_shift_threshold)
    : m_pin(pinOut)
    , min_value(pwm_min)
    , max_value(pwm_max)
    , pha_ena(false)
    , pwm(0)
    , cnt(0)
    , val(0)
    , pha(0)
    , pha_mode(random)
    , pha_thr(phase_shift_threshold)
    , pha_max(0)
    , pha_stp(0) {}

int8_t CFanCtlPWM::tick() {
    int8_t pwm_on = cnt - pha; // calculate on time (number of ticks after 0-1 pwm transition)
    if (pwm_on >= val)
        pwm_on -= max_value;
    bool o = (cnt >= pha) && (cnt < (pha + val));
    if (++cnt >= max_value) {
        cnt = 0;
        if (val != pwm) {              // pwm changed
            val = pwm;                 // update cached value
            pha = 0;                   // reset phase
            pha_max = max_value - val; // calculate maximum phase
            if ((val > 1) && (val <= pha_thr)) {
                uint8_t steps = max_value / val; // calculate number of steps
                if (steps < 3)
                    steps = 3;             // limit steps >= 3
                pha_stp = pha_max / steps; // calculate step - enable phase shifting
            } else
                pha_stp = 0; // set step to zero - disable phase shifting
        }
    #if 1
        else if (pha_stp) // pha_stp != 0 means phase shifting enabled
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
                pha = pha_max * ((float)rand() / RAND_MAX);
                break;
            }
    #endif
    }
    #if (BOARD_IS_BUDDY)
    m_pin.write(static_cast<Pin::State>(o)); // set output pin
    #else
        #error "Unknown board."
    #endif
    return pwm_on;
}

void CFanCtlPWM::set_PWM(uint8_t new_pwm) {
    if (new_pwm > max_value)
        new_pwm = max_value;
    if (new_pwm && (new_pwm < min_value))
        new_pwm = min_value;
    pwm = new_pwm;
}

void CFanCtlPWM::safeState() {
    set_PWM(max_value);
    #if (BOARD_IS_BUDDY)
    m_pin.write(Pin::State::high);
    #else
        #error "Unknown board."
    #endif
}

//------------------------------------------------------------------------------
// CFanCtlTach implementation

CFanCtlTach::CFanCtlTach(const InputPin &inputPin)
    : m_pin(inputPin) {
    input_state = false;
    tick_count = 0;
    ticks_per_second = 1000;
    edges = 0;
    pwm_sum = 0;
    rpm = 0;
    m_value_ready = false;
}

bool CFanCtlTach::tick(int8_t pwm_on) {
    bool tach = static_cast<bool>(m_pin.read());         // sample tach input pin
    bool edge = ((tach ^ input_state) && (pwm_on >= 2)); // detect edge inside pwm pulse, ignore first two sub-periods after 0-1 pwm transition
    if (edge)
        edges++;
    input_state = tach; // store current tach input state
    if (++tick_count >= ticks_per_second) {
        if (pwm_sum)
            edges = (edges * ticks_per_second) / pwm_sum; // add lost edges
        rpm = (rpm + (45 * edges)) >> 2;                  // calculate and filter rpm original formula
                                                          //rpm = (rpm + 3 * ((60 * edges) >> 2)) >> 2;
                                                          // take original rpm add 3 times new rpm - new rpm= 60*freq;
                                                          //freq=edges/2/2; edges= revolutions per second *2 *2 (2 poles motor and two edges per revolution)
        edges = 0;                                        // reset edge counter
        tick_count = 0;                                   // reset tick counter
        pwm_sum = 0;                                      // reset pwm_sum
        m_value_ready = true;                             // set value ready = measure done
    } else if (pwm_on >= 0)
        pwm_sum++; // inc pwm sum if pwm enabled
    return edge;
}

//------------------------------------------------------------------------------
// CFanCtl implementation

CFanCtl::CFanCtl(const OutputPin &pinOut, const InputPin &pinTach,
    uint8_t minPWM, uint8_t maxPWM, uint16_t minRPM, uint16_t maxRPM, uint8_t thrPWM, is_autofan_t autofan, NeedRestoreAutofan_fn need_restore, skip_tacho_t skip_tacho)
    : m_MinRPM(minRPM)
    , m_MaxRPM(maxRPM)
    , m_State(idle)
    , m_PWMValue(0)
    , is_autofan(autofan)
    , m_pwm(pinOut, minPWM, maxPWM, thrPWM)
    , m_tach(pinTach)
    , selftest_mode(false)
    , selftest_initial_pwm(0)
    , need_restore_fn(need_restore ? need_restore : DefaultNeedRestoreAutofan_fn)
    , m_skip_tacho(skip_tacho) {
    // this is not thread-safe for first look, but CFanCtl instances are global variables, so it is safe
    if (CFanCtl_count < FANCTL_MAX_FANS)
        CFanCtl_instance[CFanCtl_count++] = this;
}

void CFanCtl::tick() {
    // PWM control
    int8_t pwm_on = m_pwm.tick();
    // RPM measurement
    bool edge = 0;

    if (m_skip_tacho != skip_tacho_t::yes) {
        edge = m_tach.tick(pwm_on);
    }

    switch (m_State) {
    case idle:
        if (m_PWMValue > 0) {
            m_State = starting;
            m_Edges = 0;
            m_Ticks = 0;
        } else
            m_pwm.set_PWM(0);
        break;
    case starting:
        if (m_PWMValue == 0)
            m_State = idle;
        else {
            m_Ticks++;
            if (m_Ticks > FANCTL_START_TIMEOUT && m_skip_tacho != skip_tacho_t::yes)
                m_State = error_starting;
            else {
                m_pwm.set_PWM(m_pwm.get_max_PWM());
                if (edge)
                    m_Edges++;
                if (m_Edges >= FANCTL_START_EDGES) {
                    m_State = running;
                    m_Ticks = 0;
                }
            }
        }
        break;
    case running:
        if (m_PWMValue == 0)
            m_State = idle;
        else {
            m_pwm.set_PWM(m_PWMValue);
            if (m_Ticks < FANCTL_RPM_DELAY)
                m_Ticks++;
            else if (!getRPMIsOk() && m_skip_tacho != skip_tacho_t::yes)
                m_State = error_running;
        }
        break;
    default: // error state
        if (m_PWMValue == 0)
            m_State = idle;
        else {
            m_pwm.set_PWM(m_PWMValue);
            if (getRPMIsOk())
                m_State = running;
        }
        break;
    }
}

bool CFanCtl::setPWM(uint8_t pwm) {
    if (selftest_mode)
        return false;
    m_PWMValue = pwm;
    return true;
}

bool CFanCtl::SelftestSetPWM(uint8_t pwm) {
    if (!selftest_mode)
        return false;
    m_PWMValue = pwm;
    return true;
}

bool CFanCtl::setPhaseShiftMode(uint8_t psm) {
    if (selftest_mode)
        return false;
    m_pwm.set_PhaseShiftMode((CFanCtlPWM::PhaseShiftMode)psm);
    return true;
}

void CFanCtl::safeState() {
    m_pwm.safeState();
    selftest_mode = false;
}

bool CFanCtl::getRPMIsOk() {
    if (m_PWMValue && (getActualRPM() < m_MinRPM))
        return false;
    return true;
}

void CFanCtl::EnterSelftestMode() {
    if (selftest_mode)
        return;
    selftest_mode = true;
    selftest_initial_pwm = getPWM();
}

void CFanCtl::ExitSelftestMode() {
    if (!selftest_mode)
        return;
    selftest_mode = false;

    uint8_t pwm_to_restore;
    if (isAutoFan())
        pwm_to_restore = need_restore_fn() ? selftest_initial_pwm : 0;
    else
        pwm_to_restore = selftest_initial_pwm;

    setPWM(pwm_to_restore);
}

//------------------------------------------------------------------------------
// "C" wrapper

extern "C" {

void fanctl_tick(void) {
    for (uint8_t fan = 0; fan < CFanCtl_count; fan++)
        CFanCtl_instance[fan]->tick();
}

void fanctl_set_pwm(uint8_t fan, uint8_t pwm) {
    if (fan < CFanCtl_count)
        CFanCtl_instance[fan]->setPWM(pwm);
}

uint8_t fanctl_get_pwm(uint8_t fan) {
    if (fan < CFanCtl_count)
        return CFanCtl_instance[fan]->getPWM();
    return 0;
}

uint16_t fanctl_get_rpm(uint8_t fan) {
    if (fan < CFanCtl_count)
        return CFanCtl_instance[fan]->getActualRPM();
    return 0;
}

void fanctl_set_psm(uint8_t fan, uint8_t psm) {
    if (fan < CFanCtl_count)
        CFanCtl_instance[fan]->setPhaseShiftMode(psm);
}

uint8_t fanctl_get_psm(uint8_t fan) {
    if (fan < CFanCtl_count)
        return CFanCtl_instance[fan]->getPhaseShiftMode();
    return 0;
}
}

#endif //NEW_FANCTL
