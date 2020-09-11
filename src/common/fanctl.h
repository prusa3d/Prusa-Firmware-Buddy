// fanctl.h
#pragma once

#include <inttypes.h>
#include <stdbool.h>

enum {
    FANCTL_MAX_FANS = 2 // maximum number of fans for C wrapper functions
};

// this structure contain variables for software pwm fan control with phase-shifting
// used in class CFanCtlPWM
typedef struct _fanctl_pwm_t {
    uint8_t pin;       // output pwm pin
    uint8_t min_value; // minimum pwm value
    uint8_t max_value; // maximum pwm value
    union {
        struct {                   // flags:
            bool initialized : 1;  //  hw initialized
            bool output_state : 1; //  current pwm output state (0/1)
        };
    };
    uint8_t pwm;      // requested pwm value
    uint8_t cnt;      // pwm counter (value 0..max-1)
    uint8_t val;      // pwm value (cached during pwm cycle)
    uint8_t pha_mode; // pwm phase shift mode
    uint8_t pha_thr;  // pwm phase shift threshold (shifting will be enabled for pwm <= pha_thr)
    int8_t pha;       // pwm phase shift
    int8_t pha_max;   // pwm phase shift maximum (calculated when pwm changed)
    int8_t pha_stp;   // pwm phase shift step (calculated when pwm changed)
} fanctl_pwm_t;

// this structure contain variables for rpm measuement
// used in class CFanCtlTach
typedef struct _fanctl_tach_t {
    uint8_t pin; // input tacho pin
    union {
        struct {                  // flags:
            bool initialized : 1; //  hw initialized
            bool input_state : 1; //  last tacho input state (0/1)
        };
    };
    uint16_t tick_count;       // tick counter
    uint16_t ticks_per_second; // tacho periode in ticks
    uint16_t edges;            // number of edges in current cycle
    uint16_t pwm_sum;          // sum of ticks with pwm=1 in current cycle
    uint16_t rpm;              // calculated RPM value (filtered)
} fanctl_tach_t;

#ifdef __cplusplus

// class for software pwm control with phase-shifting
class CFanCtlPWM : private fanctl_pwm_t {
public:
    enum PhaseShiftMode {
        none,     // phase shifting disabled
        triangle, // phase shift follows triangle function
        random,   // phase shift is random (using rand)
    };

public:
    // constructor
    CFanCtlPWM(uint8_t pin_out, uint8_t pwm_min, uint8_t pwm_max, uint8_t phase_shift_threshold);

public:
    void init();   // init function - initialize hw
    int8_t tick(); // tick callback from timer interrupt
    // returns: positive number means pwm is on N ticks, negative number means pwm is off and will be switched on in -N ticks

    // getters
    inline uint8_t get_min_PWM() { return min_value; }
    inline uint8_t get_max_PWM() { return max_value; }
    inline uint8_t get_PWM() { return pwm; }
    inline PhaseShiftMode get_PhaseShiftMode() { return (PhaseShiftMode)pha_mode; }

    // setters
    void set_PWM(uint8_t new_pwm);
    inline void set_PhaseShiftMode(PhaseShiftMode new_pha_mode) { pha_mode = new_pha_mode; }
};

// class for rpm measurement
class CFanCtlTach : private fanctl_tach_t {
public:
    // constructor
    CFanCtlTach(uint8_t pin_in);

public:
    void init();              // init function - initialize hw
    bool tick(int8_t pwm_on); // tick callback from timer interrupt (currently 1kHz), returns true when edge detected
    // returns: true = tach cycle complete (used for RPM calculation)

    // getters
    inline uint16_t getRPM() { return rpm; }
};

//
class CFanCtl {
public:
    enum FanState {
        idle,           // idle - no rotation, PWM = 0%
        starting,       // starting - PWM = 100%, waiting for 4 tacho edges
        running,        // running - PWM set by setPWM(), no regulation
        measuring,      // measuring - PWM = 100%, waiting for 2 tacho edges
        error_starting, // starting error - means no feedback after timeout expired
        error_running,  // running error - means zero RPM measured (no feedback)
    };

public:
    // constructor
    CFanCtl(uint8_t pinOut, uint8_t pinTach, uint8_t minPWM, uint8_t maxPWM, uint16_t minRPM, uint16_t maxRPM, uint8_t thrPWM);

public:
    void init();               // init function - initialize hw
    void tick();               // tick callback from timer interrupt
                               // getters (in-lined)
    inline uint8_t getMinPWM() // get minimum PWM, this should be safe value for self starting
    { return m_pwm.get_min_PWM(); }
    inline uint8_t getMaxPWM() // get maximum PWM, this is value representing 100% power
    { return m_pwm.get_max_PWM(); }
    inline uint16_t getMinRPM() // get minimum RPM [n/min], this is lowest RPM that can be reached with reliable response
    { return m_pwm.get_max_PWM(); }
    inline uint16_t getMaxRPM() // get maximup RPM [n/min], this is highest RPM at 100% power
    { return m_pwm.get_max_PWM(); }
    inline FanState getState() // get fan control state
    { return m_State; }
    inline uint8_t getPWM() // get PWM value
    { return m_PWMValue; }
    inline uint16_t getActualRPM() // get actual (measured) RPM
    { return m_tach.getRPM(); }
    inline uint8_t getPhaseShiftMode() // get PhaseShiftMode
    { return m_pwm.get_PhaseShiftMode(); }
    // setters
    void setPWM(uint8_t pwm);            // set PWM value - switch to non closed-loop mode
    void setPhaseShiftMode(uint8_t psm); // set phase shift mode (none/triangle/random)
    void measure();                      // measure tacho delay
private:
    uint16_t m_MinRPM;  // minimum rpm value (set in constructor)
    uint16_t m_MaxRPM;  // maximum rpm value (set in constructor)
    FanState m_State;   // fan control state
    uint8_t m_PWMValue; // current pwm value
    uint8_t m_Edges;    // edge counter - used for starting and measurement
    uint8_t m_Ticks;    // tick counter - used for starting and measurement

    CFanCtlPWM m_pwm;
    CFanCtlTach m_tach;
};

extern "C" {
#endif //__cplusplus

// C wrapper functions
extern void fanctl_init(void);                        // init for all fanctl instances is done using this function in appmain.cpp
extern void fanctl_tick(void);                        // tick for all fanctl instances is done using this function in appmain.cpp
extern void fanctl_set_pwm(uint8_t fan, uint8_t pwm); // set requested PWM value
extern uint8_t fanctl_get_pwm(uint8_t fan);           // get requested PWM value
extern uint16_t fanctl_get_rpm(uint8_t fan);          // get actual RPM value
extern void fanctl_set_psm(uint8_t fan, uint8_t psm); // set PhaseShiftMode
extern uint8_t fanctl_get_psm(uint8_t fan);           // get PhaseShiftMode

#ifdef __cplusplus
}
#endif //__cplusplus
