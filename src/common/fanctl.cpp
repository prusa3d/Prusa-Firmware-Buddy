// fanctl.cpp

#include "fanctl.h"
#include "stm32f4xx_hal.h"
#include "gpio.h"

// global variables for C wrapper
static uint8_t CFanCtl_count = 0;                  // number of instances
static CFanCtl *CFanCtl_instance[FANCTL_MAX_FANS]; // array of pointers to instances

//------------------------------------------------------------------------------
// CFanCtlPWM implementation

CFanCtlPWM::CFanCtlPWM(uint8_t pin_out, uint8_t pwm_min, uint8_t pwm_max) {
    pin = pin_out;
    min_value = pwm_min;
    max_value = pwm_max;
    initialized = false;
    output_state = false;
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
    initialized = true;
}

int8_t CFanCtlPWM::tick() {
    if (!initialized)          // ?not initialised
        return -1;             // return -1 means pwm output is off
    int8_t pwm_on = cnt - pha; // calculate on time (number of ticks after 0-1 pwm transition)
    if (pwm_on >= val)
        pwm_on -= max_value;
    bool o = (cnt >= pha) && (cnt < (pha + val));
    if (++cnt >= max_value) {
        cnt = 0;
        if (val != pwm) { // pwm changed
            val = pwm;    // update cached value
            pha = 0;      // reset phase
            if ((val > 1) && (val <= (max_value / 2))) {
                pha_max = max_value - val;
                uint8_t steps = max_value / val; // calculate number of steps
                if (steps < 3)
                    steps = 3;               // limit steps >= 3
                pha_stp = max_value / steps; // calculate step - enable phase shifting
            } else
                pha_stp = 0; // set step to zero - disable phase shifting
        }
#if 1
        else if (pha_stp) { // phase-shifting enabled
            pha += pha_stp;
            if (pha >= pha_max) {
                pha_stp = -pha_stp;
                pha = pha_max;
            } else if (pha < 0) {
                pha_stp = -pha_stp;
                pha = 0;
            }
        }
#endif
    }
    if (o != output_state)        // pwm output changed?
        gpio_set(pin, o ? 1 : 0); // set output pin
    output_state = o;             // store pwm output state
    return pwm_on;
}

void CFanCtlPWM::set_PWM(uint8_t new_pwm) {
    if (new_pwm > max_value)
        new_pwm = max_value;
    if (new_pwm && (new_pwm < min_value))
        new_pwm = min_value;
    pwm = new_pwm;
}

//------------------------------------------------------------------------------
// CFanCtlTach implementation

CFanCtlTach::CFanCtlTach(uint8_t pin_in) {
    pin = pin_in;
    initialized = false;
    input_state = false;
    tick_count = 0;
    ticks_per_second = 1000;
    edges = 0;
    pwm_sum = 0;
    rpm = 0;
}

void CFanCtlTach::init() {
    gpio_init(pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
    initialized = true;
}

void CFanCtlTach::tick(int8_t pwm_on) {
    if (!initialized) {
        return;
    }
    bool tach = gpio_get(pin) ? true : false;  // sample tach input pin
    if ((tach ^ input_state) && (pwm_on >= 2)) // detect edge inside pwm pulse, ignore first two sub-periods after 0-1 pwm transition
        edges++;
    input_state = tach; // store current tach input state
    if (++tick_count >= ticks_per_second) {
        if (pwm_sum)
            edges *= 1 + ((ticks_per_second - pwm_sum) / pwm_sum); // add lost edges
        rpm = (rpm + 3 * ((60 * edges) >> 2)) >> 2;                // calculate and filter rpm
        edges = 0;                                                 // reset edge counter
        tick_count = 0;                                            // reset tick counter
        pwm_sum = 0;                                               // reset pwm_sum
    } else if (pwm_on >= 0)
        pwm_sum++; // inc pwm sum if pwm enabled
}

//------------------------------------------------------------------------------
// CFanCtl implementation

CFanCtl::CFanCtl(uint8_t pinOut, uint8_t pinTach, uint8_t minPWM, uint8_t maxPWM, uint16_t minRPM, uint16_t maxRPM)
    : m_pwm(pinOut, minPWM, maxPWM)
    , m_tach(pinTach) {
    m_MinRPM = minRPM;
    m_MaxRPM = maxRPM;
    m_State = idle;
    // this is not thread-safe for first look, but CFanCtl instances are global variables, so it is safe
    if (CFanCtl_count < FANCTL_MAX_FANS)
        CFanCtl_instance[CFanCtl_count++] = this;
}

void CFanCtl::init() {
    m_pwm.init();
    m_tach.init();
}

void CFanCtl::tick() {
    // PWM control
    int8_t pwm_on = m_pwm.tick();
    // RPM measurement
    m_tach.tick(pwm_on);
}

void CFanCtl::setPWM(uint8_t pwm) {
    m_pwm.set_PWM(pwm);
}

//------------------------------------------------------------------------------
// "C" wrapper

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
}
