// fanctl.cpp

#include "fanctl.h"
#include "stm32f4xx_hal.h"
#include "gpio.h"

// global variables for C wrapper
static uint8_t CFanCtl_count = 0;                  // number of instances
static CFanCtl *CFanCtl_instance[FANCTL_MAX_FANS]; // array of pointers to instances

CFanCtlPWM::CFanCtlPWM(uint8_t pin_out, uint8_t pwm_min, uint8_t pwm_max) {
    pin = pin_out;
    min = pwm_min;
    max = pwm_max;
    ini = false;
    out = false;
    pha_ena = false;
    pwm = 0;
    cnt = 0;
    val = 0;
    pha = 0;
    pha_max = 0;
    pha_stp = 0;
}

void CFanCtlPWM::init() {
    gpio_set(pin, 0);
    gpio_init(pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    ini = true;
}

void CFanCtlPWM::tick() {
    if (!ini)
        return;
    bool o = (cnt >= pha) && (cnt < (pha + val));
    if (++cnt >= max) {
        cnt = 0;
        if (val != pwm) {
            val = pwm; // update cached value
            pha = 0;
            pha_max = max - val;
            pha_stp = 1;
        } else { // phase-shifting enabled
            if (pha_stp > 0) {
                if (++pha >= pha_max) {
                    pha_stp = -1;
                    pha--;
                }
            }
            if (pha_stp < 0) {
                if (pha-- == 0) {
                    pha_stp = 1;
                    pha = 0;
                }
            }
        }
    }
    if (o != out)                 // pwm output changed?
        gpio_set(pin, o ? 1 : 0); // set output pin
    out = o;                      // store pwm output state
}

void CFanCtlPWM::set_PWM(uint8_t new_pwm) {
    if (new_pwm > max)
        new_pwm = max;
    if (new_pwm && (new_pwm < min))
        new_pwm = min;
    pwm = new_pwm;
}

CFanCtl::CFanCtl(uint8_t pinOut, uint8_t pinTach, uint8_t minPWM, uint8_t maxPWM, uint16_t minRPM, uint16_t maxRPM)
    : m_pwm(pinOut, minPWM, maxPWM) {
    m_pinTach = pinTach;
    m_MinRPM = minRPM;
    m_MaxRPM = maxRPM;
    m_State = idle;
    m_ActualRPM = 0;
    m_TargetRPM = 0;
    m_tach = false;
    m_tach_edge = 0;
    m_fan_rpm100 = 0;
    m_tach_time = 0;
    m_tach_max = 600;
    // this is not thread-safe for first look, but CFanCtl instances are global variables, so it is safe
    if (CFanCtl_count < FANCTL_MAX_FANS)
        CFanCtl_instance[CFanCtl_count++] = this;
}

void CFanCtl::init() {
    m_pwm.init();
    gpio_init(m_pinTach, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
}

void CFanCtl::tick() {
    m_pwm.tick();
    //RPM Measure
    /*    bool tach = gpio_get(m_pinTach) ? true : false; // sample tach input pin
    bool edge = (m_tach ^ tach) & m_pwm_out;        // detect edge - ignore edges for pwmOut = 0
    m_tach = tach;                                  // store current tach input state
    if (m_pwm_cnt > (2 + m_pwm_pha))                // ignore first two sub-periods after 0-1 pwm transition
        if ((m_tach_edge < 255) && edge)
            m_tach_edge++;
    if (++m_tach_time >= m_tach_max) {
        m_fan_rpm100 = ((uint16_t)m_tach_edge + m_fan_rpm100) >> 1;
        m_tach_edge = 0; // reset edge counter
        m_tach_time = 0; // reset tach cycle counter
        m_ActualRPM = 60 * (uint16_t)m_fan_rpm100 / 2;
    }*/
}

void CFanCtl::setPWM(uint8_t pwm) {
    m_pwm.set_PWM(pwm);
}

void CFanCtl::setTargetRPM(uint16_t targetRPM) {
    if (targetRPM > m_MaxRPM)
        targetRPM = m_MaxRPM;
    if (targetRPM && (targetRPM < m_MinRPM))
        targetRPM = m_MinRPM;
    m_TargetRPM = targetRPM;
    m_pwm.set_PWM(0);
}

extern "C" {

void fanctl_init(void) {
    for (uint8_t fan = 0; fan < CFanCtl_count; fan++)
        CFanCtl_instance[fan]->init();
}

void fanctl_tick(void) {
    for (uint8_t fan = 0; fan < CFanCtl_count; fan++)
        CFanCtl_instance[fan]->tick();
}

void fanctl_set_pwm(uint8_t fan, uint8_t pwm) {
    if (fan < CFanCtl_count)
        CFanCtl_instance[fan]->setPWM(pwm);
}

void fanctl_set_rpm(uint8_t fan, uint16_t rpm) {
    if (fan < CFanCtl_count)
        CFanCtl_instance[fan]->setTargetRPM(rpm);
}
}
