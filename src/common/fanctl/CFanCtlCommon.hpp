#pragma once

#include <stdint.h>
#include <printers.h>
#include <option/xl_enclosure_support.h>
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
        running = 3, // running - PWM set by setPWM(), no regulation
        error_starting = -1, // starting error - means no feedback after timeout expired
        error_running = -2, // running error - means zero RPM measured (no feedback)
    };

    inline uint8_t getMinRPM() const { return min_rpm; }
    inline uint8_t getMaxRPM() const { return max_rpm; }
    inline uint16_t getMaxPWM() const { return 255; }

    virtual uint16_t getMinPWM() const = 0;
    virtual FanState getState() const = 0;
    virtual uint8_t getPWM() const = 0;
    virtual uint16_t getActualRPM() const = 0;
    virtual bool getRPMIsOk() const = 0;
    virtual bool getRPMMeasured() const = 0;

    // Accepts uint16_t only because Puppies use (uint16_t)-1 as an "auto-fan" signal. PWM is still 0-255.
    virtual bool setPWM(uint16_t pwm) = 0;

    inline bool isSelftest() { return selftest_mode; }
    constexpr void disable_autocontrol() { autocontrol_enabled = false; }
    constexpr void enable_autocontrol() { autocontrol_enabled = true; }
    virtual void enterSelftestMode() = 0;
    virtual void exitSelftestMode() = 0;
    virtual bool selftestSetPWM(uint8_t pwm) = 0;

    virtual bool is_fan_ok() const;

    virtual void tick() = 0;

protected:
    const uint16_t min_rpm; // minimum rpm value (set in constructor)
    const uint16_t max_rpm; // maximum rpm value (set in constructor)
    bool selftest_mode { false };
    bool autocontrol_enabled { true };
    std::atomic<uint8_t> selftest_initial_pwm { 0 };
};

extern void record_fanctl_metrics();

// FANCTLPRINT - printing fan
inline constexpr uint8_t FANCTLPRINT_PWM_MIN = 10; // min duty cycle length 10 / 50 = 0.2 = 20%
inline constexpr uint8_t FANCTLPRINT_PWM_MAX = 50; // 1000Hz / 50 = 20Hz PWM cycle
#if PRINTER_IS_PRUSA_MK4()
inline constexpr uint16_t FANCTLPRINT_RPM_MIN = 90; // Dynamic PWM enables lower RPM
#else
inline constexpr uint16_t FANCTLPRINT_RPM_MIN = 150;
#endif
inline constexpr uint16_t FANCTLPRINT_RPM_MAX =
#if (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_COREONE())
    6850
#elif PRINTER_IS_PRUSA_MINI()
    5000
#else
    #error "You need to specify printfans max RPM"
#endif
    ;
inline constexpr uint8_t FANCTLPRINT_PWM_THR = 20;

// On Mk3 the printer would ignore rpm measurements if the pwm was under 30%.
// Because some of the printers have a really weak print fan, it would cause
// MK3.5 users to get print fan errors on low pwm, that wouldn't happend on MK3.
// Sadly since we doing pwm differently we are not able to set it to 30% exactly,
// but rather we round to nearest int:
// <= 32% - ignore RPM measurement
// >= 33% - will trigger print fan error if the pwm is too low (FANCTLPRINT_RPM_MIN)
inline constexpr uint8_t FANCTLPRINT_MIN_PWM_TO_MEASURE_RPM =
#if PRINTER_IS_PRUSA_MK3_5()
    FANCTLPRINT_PWM_MAX * 0.3;
#else
    0;
#endif

// FANCTLHEATBREAK - heatbreak fan
inline constexpr uint8_t FANCTLHEATBREAK_PWM_MIN = 0;
inline constexpr uint8_t FANCTLHEATBREAK_PWM_MAX = 50;
inline constexpr uint16_t FANCTLHEATBREAK_RPM_MIN = 1000;
inline constexpr uint16_t FANCTLHEATBREAK_RPM_MAX =
#if (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_COREONE())
    15180
#elif PRINTER_IS_PRUSA_MINI()
    8000
#else
    #error "You need to specify printfans max RPM"
#endif
    ;
inline constexpr uint8_t FANCTLHEATBREAK_PWM_THR = 20;
inline constexpr uint8_t FANCTLHEATBREAK_MIN_PWM_TO_MEASURE_RPM = 0;

// FANCTLENCLOSURE - enclosure fan
#if XL_ENCLOSURE_SUPPORT()
inline constexpr uint8_t FANCTLENCLOSURE_PWM_MIN = 0;
inline constexpr uint8_t FANCTLENCLOSURE_PWM_MAX = 255;
inline constexpr uint16_t FANCTLENCLOSURE_RPM_MIN = 600;
inline constexpr uint16_t FANCTLENCLOSURE_RPM_MAX = 2700;
#endif // XL_ENCLOSURE_SUPPORT
