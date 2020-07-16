// fanctl.h
#ifndef _FANCTL_H
#define _FANCTL_H

#include <inttypes.h>
#include <stdbool.h>

#define FANCTL_MAX_FANS 2 // maximum number of fans for C wrapper functions

// this structure contain variables for software pwm fan control with phase-shifting
typedef struct _fanctl_pwm_t {
    uint8_t pin : 8; // output pwm pin
    uint8_t min : 8; // minimum pwm value
    uint8_t max : 8; // maximum pwm value
    union {
        struct {              // flags:
            bool ini : 1;     //  hw initialized
            bool out : 1;     //  current pwm output state (0/1)
            bool pha_ena : 1; //  phase shift enabled
        };
        uint8_t flg : 8; // minimum pwm value
    };
    uint8_t pwm : 8;     // requested pwm value
    uint8_t cnt : 8;     // pwm counter (value 0..max-1)
    uint8_t val : 8;     // pwm value (cached during pwm cycle)
    uint8_t pha : 8;     // pwm phase shift
    uint8_t pha_max : 8; // pwm phase shift maximum (calculated when pwm changed)
    int8_t pha_stp : 8;  // pwm phase shift step (calculated when pwm changed)
} fanctl_pwm_t;

typedef struct _fanctl_tach_t {
    uint8_t pin : 8; // input tacho pin
} fanctl_tach_t;

#ifdef __cplusplus

class CFanCtlPWM : private fanctl_pwm_t {
public:
    // constructor
    CFanCtlPWM(uint8_t pin_out, uint8_t pwm_min, uint8_t pwm_max);

public:
    void init(); // init function - initialize hw
    void tick(); // tick callback from timer interrupt (currently 1kHz)
                 // getters
    inline uint8_t get_min_PWM() { return min; }
    inline uint8_t get_max_PWM() { return max; }
    inline uint8_t get_PWM() { return pwm; }
    // setters
    void set_PWM(uint8_t new_pwm);
};

class CFanCtl {
public:
    enum FanState {
        idle,             // idle - no rotation, PWM = 0%
        starting,         // starting - PWM = 100%, waiting for tacho pulse
        running,          // running - PWM set by setPWM(), no regulation
        closedloop,       // closed-loop - PWM set by internal regulator
        error_starting,   // starting error - means no feedback after timeout expired
        error_running,    // running error - means zero RPM measured (no feedback)
        error_closedloop, // closed-loop error - means unable to reach target rpm with desired tolerance limit
    };

public:
    // constructor
    CFanCtl(uint8_t pinOut, uint8_t pinTach, uint8_t minPWM, uint8_t maxPWM, uint16_t minRPM, uint16_t maxRPM);

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
    { return m_pwm.get_PWM(); }
    inline uint16_t getActualRPM() // get actual (measured) RPM
    { return m_ActualRPM; }
    inline uint16_t getTargetRPM() // get target RPM
    { return m_TargetRPM; }
    // setters
    void setPWM(uint8_t pwm);              // set PWM value - switch to non closed-loop mode
    void setTargetRPM(uint16_t targetRPM); // set target RPM - switch to closed-loop mode
private:
    uint8_t m_pinTach;    // input tacho pin (set in constructor)
    uint16_t m_MinRPM;    // minimum rpm value (set in constructor)
    uint16_t m_MaxRPM;    // maximum rpm value (set in constructor)
    FanState m_State;     // fan control state
    uint16_t m_ActualRPM; // actual rpm value (measured)
    uint16_t m_TargetRPM; // target rpm value for closed-loop mode
    bool m_tach;          // cached tach input state (0/1)
    uint8_t m_tach_edge;  // edge counter
    uint8_t m_fan_rpm100; // rpm value (filtered)
    uint16_t m_tach_time; // tach cycle counter
    uint16_t m_tach_max;  // tach cycle length

    CFanCtlPWM m_pwm;
};

extern "C" {
#endif //__cplusplus

// C wrapper functions
extern void fanctl_init(void);                         // init for all fanctl instances is done using this function in appmain.cpp
extern void fanctl_tick(void);                         // tick for all fanctl instances is done using this function in appmain.cpp
extern void fanctl_set_pwm(uint8_t fan, uint8_t pwm);  // set requested PWM value
extern uint8_t fanctl_get_pwm(uint8_t fan);            // get requested PWM value
extern void fanctl_set_rpm(uint8_t fan, uint16_t rpm); // set requested RPM value
extern uint16_t fanctl_get_rpm(uint8_t fan);           // get requested RPM value

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _FANCTL_H
