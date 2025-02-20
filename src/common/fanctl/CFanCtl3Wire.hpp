// fanctl.hpp
#pragma once

#include <stdbool.h>
#include "Pin.hpp"
#include "CFanCtlCommon.hpp"
#include <option/has_love_board.h>

enum {
    FANCTL_MAX_FANS = 2, // maximum number of fans for C wrapper functions
    FANCTL_START_TIMEOUT = 2000, //
    FANCTL_START_EDGES = 4, //
    FANCTL_RPM_DELAY = 5000, //
};

// this structure contain variables for rpm measuement
// used in class CFanCtlTach
struct fanctl_tach_t {
    union {
        struct { // flags:
            bool input_state : 1; //  last tacho input state (0/1)
        };
    };
    uint16_t tick_count; // tick counter
    uint16_t ticks_per_second; // tacho periode in ticks
    uint16_t edges; // number of edges in current cycle
    uint16_t pwm_sum; // sum of ticks with pwm=1 in current cycle
    uint16_t rpm; // calculated RPM value (filtered)
    bool m_value_ready; // measure RPM done
};

// class for software pwm control with phase-shifting
class CFanCtlPWM {
public:
    enum PhaseShiftMode : uint8_t {
        none, // phase shifting disabled
        triangle, // phase shift follows triangle function
        random, // phase shift is random (using rand)
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
    inline PhaseShiftMode get_PhaseShiftMode() const { return pha_mode; }

    // setters
    inline void set_min_PWM(uint8_t val) { min_value = val; }
    inline void set_max_PWM(uint8_t val) { max_value = val; }
    inline void set_PhaseShiftThr(uint8_t val) { pha_thr = val; }
    void set_PWM(uint8_t new_pwm);
    inline void set_PhaseShiftMode(PhaseShiftMode new_pha_mode) { pha_mode = new_pha_mode; }
    void safeState();

private:
    const buddy::hw::OutputPin &m_pin;
    uint8_t min_value; // minimum pwm value
    uint8_t max_value; // maximum pwm value
    union {
        struct { // flags:
            bool pha_ena : 1; //  phase shift enabled
        };
        uint8_t flags; // flags as uint8
    };
    uint8_t pwm; // requested pwm value
    uint8_t cnt; // pwm counter (value 0..max-1)
    uint8_t val; // pwm value (cached during pwm cycle)
    int8_t pha; // pwm phase shift
    PhaseShiftMode pha_mode; // pwm phase shift mode
    uint8_t pha_thr; // pwm phase shift threshold (shifting will be enabled for pwm <= pha_thr)
    int8_t pha_max; // pwm phase shift maximum (calculated when pwm changed)
    int8_t pha_stp; // pwm phase shift step (calculated when pwm changed)
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
class CFanCtl3Wire : public CFanCtlCommon {

public:
    // constructor
    CFanCtl3Wire(const buddy::hw::OutputPin &pinOut, const buddy::hw::InputPin &pinTach, uint8_t minPWM, uint8_t maxPWM,
        uint16_t minRPM, uint16_t maxRPM, uint8_t thrPWM, is_autofan_t autofan, skip_tacho_t skip_tacho, uint8_t min_pwm_to_measure_rpm);

    virtual void tick() override; // tick callback from timer interrupt

    // getters
    virtual uint16_t getMinPWM() const override // get minimum PWM, this should be safe value for self starting
    { return m_pwm.get_min_PWM(); }
    virtual FanState getState() const override // get fan control state
    { return m_State; }
    virtual uint8_t getPWM() const override // get PWM value
    { return unscalePWM(m_PWMValue); }
    uint16_t getActualRPM() const final // get actual (measured) RPM
    {
#if HAS_LOVE_BOARD() && !PRINTER_IS_PRUSA_iX()
        if (autocontrol_enabled) {
            return m_tach.getRPM();
        }
        return 8669;
#else
        return m_tach.getRPM();
#endif
    }
    CFanCtlPWM::PhaseShiftMode getPhaseShiftMode() const // get PhaseShiftMode
    { return m_pwm.get_PhaseShiftMode(); }
    virtual bool getRPMIsOk() const override;
    inline bool isAutoFan() const // get fan type
    { return is_autofan == is_autofan_t::yes; }

    virtual bool getRPMMeasured() const override { return m_tach.getValueReady(); }
    inline skip_tacho_t getSkipTacho() const { return m_skip_tacho; }

    uint16_t scalePWM(uint16_t pwm) const; // scale pwm from 0-255 to range used by this instance
    uint16_t unscalePWM(uint16_t pwm) const; // unscale pwm from range used by this instance to 0-255

    // setters
    virtual bool setPWM(uint16_t pwm) override; // set PWM value - switch to non closed-loop mode
    bool setPhaseShiftMode(CFanCtlPWM::PhaseShiftMode psm); // set phase shift mode (none/triangle/random)
    void safeState();

    inline void setSkipTacho(skip_tacho_t skip_tacho) {
        m_skip_tacho = skip_tacho;
        m_tach.setValueReady(false);
    }

    virtual void enterSelftestMode() override;
    virtual void exitSelftestMode() override;
    virtual bool selftestSetPWM(uint8_t pwm) override; // sets pwm in selftest, doesn't work outside selftest

protected:
    uint16_t m_Ticks; // tick counter - used for starting and measurement
    uint16_t m_Result;
    FanState m_State; // fan control state
    uint8_t m_PWMValue; // current pwm value
    uint8_t m_Edges; // edge counter - used for starting and measurement
    uint8_t min_pwm_to_measure_rpm;
    is_autofan_t is_autofan; // autofan restores temp differently (used in selftest)
    CFanCtlPWM m_pwm;
    CFanCtlTach m_tach;

    skip_tacho_t m_skip_tacho; // skip tacho measure
};
