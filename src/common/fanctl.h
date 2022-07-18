// fanctl.h
#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include "Pin.hpp"

enum {
    FANCTL_MAX_FANS = 2,         // maximum number of fans for C wrapper functions
    FANCTL_START_TIMEOUT = 2000, //
    FANCTL_START_EDGES = 4,      //
    FANCTL_RPM_DELAY = 5000,     //
};

// this structure contain variables for rpm measuement
// used in class CFanCtlTach
typedef struct _fanctl_tach_t {
    union {
        struct {                  // flags:
            bool input_state : 1; //  last tacho input state (0/1)
        };
    };
    uint16_t tick_count;       // tick counter
    uint16_t ticks_per_second; // tacho periode in ticks
    uint16_t edges;            // number of edges in current cycle
    uint16_t pwm_sum;          // sum of ticks with pwm=1 in current cycle
    uint16_t rpm;              // calculated RPM value (filtered)
    bool m_value_ready;        // measure RPM done
} fanctl_tach_t;

#ifdef __cplusplus

// class for software pwm control with phase-shifting
class CFanCtlPWM {
public:
    enum PhaseShiftMode : uint8_t {
        none,     // phase shifting disabled
        triangle, // phase shift follows triangle function
        random,   // phase shift is random (using rand)
    };

public:
    // constructor
    CFanCtlPWM(const buddy::hw::OutputPin &pinOut, uint8_t pwm_min, uint8_t pwm_max, uint8_t phase_shift_threshold);

public:
    int8_t tick(); // tick callback from timer interrupt
    // returns: positive number means pwm is on N ticks, negative number means pwm is off and will be switched on in -N ticks

    // getters
    inline uint8_t get_min_PWM() const { return min_value; }
    inline uint8_t get_max_PWM() const { return max_value; }
    inline uint8_t get_PWM() const { return pwm; }
    inline PhaseShiftMode get_PhaseShiftMode() const { return (PhaseShiftMode)pha_mode; }

    // setters
    void set_PWM(uint8_t new_pwm);
    inline void set_PhaseShiftMode(PhaseShiftMode new_pha_mode) { pha_mode = new_pha_mode; }
    void safeState();

private:
    const buddy::hw::OutputPin &m_pin;
    const uint8_t min_value; // minimum pwm value
    const uint8_t max_value; // maximum pwm value
    union {
        struct {              // flags:
            bool pha_ena : 1; //  phase shift enabled
        };
        uint8_t flags; // flags as uint8
    };
    uint8_t pwm;      // requested pwm value
    uint8_t cnt;      // pwm counter (value 0..max-1)
    uint8_t val;      // pwm value (cached during pwm cycle)
    int8_t pha;       // pwm phase shift
    uint8_t pha_mode; // pwm phase shift mode
    uint8_t pha_thr;  // pwm phase shift threshold (shifting will be enabled for pwm <= pha_thr)
    int8_t pha_max;   // pwm phase shift maximum (calculated when pwm changed)
    int8_t pha_stp;   // pwm phase shift step (calculated when pwm changed)
};

// class for rpm measurement
class CFanCtlTach : private fanctl_tach_t {
public:
    // constructor
    CFanCtlTach(const buddy::hw::InputPin &inputPin);

public:
    bool tick(int8_t pwm_on); // tick callback from timer interrupt (currently 1kHz), returns true when edge detected
    // returns: true = tach cycle complete (used for RPM calculation)

    // getters
    inline uint16_t getRPM() const { return rpm; }

    inline bool getValueReady() const { return m_value_ready; }

    // setters
    inline void setValueReady(bool value_ready) { m_value_ready = value_ready; }

private:
    const buddy::hw::InputPin &m_pin;
};

enum class is_autofan_t : bool {
    no,
    yes
};

enum class skip_tacho_t : bool {
    no,
    yes
};

//
class CFanCtl {
public:
    using NeedRestoreAutofan_fn = bool (*)();
    static bool DefaultNeedRestoreAutofan_fn() { return true; } //set in case someone pass nullptr in ctor
    enum FanState : uint8_t {
        idle,           // idle - no rotation, PWM = 0%
        starting,       // starting - PWM = 100%, waiting for 4 tacho edges
        running,        // running - PWM set by setPWM(), no regulation
        error_starting, // starting error - means no feedback after timeout expired
        error_running,  // running error - means zero RPM measured (no feedback)
    };

public:
    // constructor
    CFanCtl(const buddy::hw::OutputPin &pinOut, const buddy::hw::InputPin &pinTach, uint8_t minPWM, uint8_t maxPWM,
        uint16_t minRPM, uint16_t maxRPM, uint8_t thrPWM, is_autofan_t autofan, NeedRestoreAutofan_fn need_restore, skip_tacho_t skip_tacho);

public:
    void tick(); // tick callback from timer interrupt

    // getters (in-lined)
    inline uint8_t getMinPWM() const // get minimum PWM, this should be safe value for self starting
    { return m_pwm.get_min_PWM(); }
    inline uint8_t getMaxPWM() const // get maximum PWM, this is value representing 100% power
    { return m_pwm.get_max_PWM(); }
    inline uint16_t getMinRPM() const // get minimum RPM [n/min], this is lowest RPM that can be reached with reliable response
    { return m_MinRPM; }
    inline uint16_t getMaxRPM() const // get maximup RPM [n/min], this is highest RPM at 100% power
    { return m_MaxRPM; }
    inline FanState getState() const // get fan control state
    { return m_State; }
    inline uint8_t getPWM() const // get PWM value
    { return m_PWMValue; }
    inline uint16_t getActualRPM() const // get actual (measured) RPM
    { return m_tach.getRPM(); }
    inline uint8_t getPhaseShiftMode() const // get PhaseShiftMode
    { return m_pwm.get_PhaseShiftMode(); }
    bool getRPMIsOk();
    inline bool isAutoFan() const // get fan type
    { return is_autofan == is_autofan_t::yes; }

    inline bool getRPMMeasured() const { return m_tach.getValueReady(); }
    inline skip_tacho_t getSkipTacho() const { return m_skip_tacho; }

    // setters
    bool setPWM(uint8_t pwm);            // set PWM value - switch to non closed-loop mode
    bool setPhaseShiftMode(uint8_t psm); // set phase shift mode (none/triangle/random)
    void safeState();

    inline void setSkipTacho(skip_tacho_t skip_tacho) {
        m_skip_tacho = skip_tacho;
        m_tach.setValueReady(false);
    }

    void EnterSelftestMode();
    void ExitSelftestMode();
    bool SelftestSetPWM(uint8_t pwm); // sets pwm in selftest, doesn't work outside selftest
private:
    const uint16_t m_MinRPM; // minimum rpm value (set in constructor)
    const uint16_t m_MaxRPM; // maximum rpm value (set in constructor)
    uint16_t m_Ticks;        // tick counter - used for starting and measurement
    uint16_t m_Result;
    FanState m_State;        // fan control state
    uint8_t m_PWMValue;      // current pwm value
    uint8_t m_Edges;         // edge counter - used for starting and measurement
    is_autofan_t is_autofan; // autofan restores temp differently (used in selftest)
    CFanCtlPWM m_pwm;
    CFanCtlTach m_tach;

    bool selftest_mode;
    uint8_t selftest_initial_pwm;
    NeedRestoreAutofan_fn need_restore_fn;
    skip_tacho_t m_skip_tacho; //skip tacho measure
};

extern "C" {
#endif //__cplusplus

// C wrapper functions
extern void fanctl_tick(void);                        // tick for all fanctl instances is done using this function in appmain.cpp
extern void fanctl_set_pwm(uint8_t fan, uint8_t pwm); // set requested PWM value
extern uint8_t fanctl_get_pwm(uint8_t fan);           // get requested PWM value
extern uint16_t fanctl_get_rpm(uint8_t fan);          // get actual RPM value
extern void fanctl_set_psm(uint8_t fan, uint8_t psm); // set PhaseShiftMode
extern uint8_t fanctl_get_psm(uint8_t fan);           // get PhaseShiftMode

#ifdef __cplusplus
}
#endif //__cplusplus
