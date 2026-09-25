#pragma once

#include <stdint.h>
#include <atomic>

class CFanCtlCommon {
public:
    CFanCtlCommon(uint16_t min_rpm, uint16_t max_rpm)
        : min_rpm(min_rpm)
        , max_rpm(max_rpm)
        , selftest_mode(false) {};

    enum FanState : int8_t {
        idle = 0, // idle - no rotation, PWM = 0%
        starting = 1, // starting - PWM = 100%, waiting for 4 tacho edges
        rpm_stabilization = 2, // tick delay for reaching wanted rpm
        running = 3, // running - PWM set by set_pwm(), no regulation
        error_starting = -1, // starting error - means no feedback after timeout expired
        error_running = -2, // running error - means zero RPM measured (no feedback)
    };

    inline uint16_t get_min_rpm() const { return min_rpm; }
    inline uint16_t get_max_rpm() const { return max_rpm; }
    inline uint16_t get_max_pwm() const { return 255; }

    virtual uint16_t get_min_pwm() const = 0;
    virtual FanState get_state() const = 0;
    virtual uint8_t get_pwm() const = 0;
    virtual uint16_t get_actual_rpm() const = 0;
    virtual bool get_rpm_is_ok() const = 0;
    virtual bool get_rpm_measured() const = 0;

    // Accepts uint16_t only because Puppies use (uint16_t)-1 as an "auto-fan" signal. PWM is still 0-255.
    virtual bool set_pwm(uint16_t pwm) = 0;

    inline bool is_selftest() const { return selftest_mode; }
    inline bool is_autocontrol_enabled() const { return autocontrol_enabled; }
    constexpr void disable_autocontrol() { autocontrol_enabled = false; }
    constexpr void enable_autocontrol() { autocontrol_enabled = true; }
    virtual void enter_selftest_mode() = 0;
    virtual void exit_selftest_mode() = 0;
    virtual bool selftest_set_pwm(uint8_t pwm) = 0;

    virtual bool is_fan_ok() const;

    virtual void tick() = 0;

    virtual void safe_state() = 0;

protected:
    const uint16_t min_rpm; // minimum rpm value (set in constructor)
    const uint16_t max_rpm; // maximum rpm value (set in constructor)
    bool selftest_mode { false };
    bool autocontrol_enabled { true };
    std::atomic<uint8_t> selftest_initial_pwm { 0 };
};
