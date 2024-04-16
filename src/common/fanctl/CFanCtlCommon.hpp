#pragma once

#include <stdint.h>
#include <printers.h>
#include <device/board.h>

class CFanCtlCommon {
public:
    CFanCtlCommon(uint16_t min_rpm, uint16_t max_rpm)
        : min_rpm(min_rpm)
        , max_rpm(max_rpm)
        , selftest_mode(false) {};
    virtual ~CFanCtlCommon() {}

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
    virtual bool getRPMIsOk() = 0;
    virtual bool getRPMMeasured() const = 0;
    virtual bool setPWM(uint16_t pwm) = 0;

    inline bool isSelftest() { return selftest_mode; }
    virtual void enterSelftestMode() = 0;
    virtual void exitSelftestMode() = 0;
    virtual bool selftestSetPWM(uint8_t pwm) = 0;

    virtual void tick() = 0;

protected:
    const uint16_t min_rpm; // minimum rpm value (set in constructor)
    const uint16_t max_rpm; // maximum rpm value (set in constructor)
    bool selftest_mode;
};

extern void record_fanctl_metrics();

// FANCTLPRINT - printing fan
inline constexpr uint8_t FANCTLPRINT_PWM_MIN = 10;
inline constexpr uint8_t FANCTLPRINT_PWM_MAX = 50;
inline constexpr uint16_t FANCTLPRINT_RPM_MIN = 150;
inline constexpr uint16_t FANCTLPRINT_RPM_MAX =
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX || PRINTER_IS_PRUSA_XL)
    6850
#else
    5000
#endif
    ;
inline constexpr uint8_t FANCTLPRINT_PWM_THR = 20;

// FANCTLHEATBREAK - heatbreak fan
inline constexpr uint8_t FANCTLHEATBREAK_PWM_MIN = 0;
inline constexpr uint8_t FANCTLHEATBREAK_PWM_MAX = 50;
inline constexpr uint16_t FANCTLHEATBREAK_RPM_MIN = 1000;
inline constexpr uint16_t FANCTLHEATBREAK_RPM_MAX =
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX || PRINTER_IS_PRUSA_XL)
    15180
#else
    8000
#endif
    ;
inline constexpr uint8_t FANCTLHEATBREAK_PWM_THR = 20;

// FANCTLENCLOSURE - enclosure fan
#if XL_ENCLOSURE_SUPPORT()
inline constexpr uint8_t FANCTLENCLOSURE_PWM_MIN = 0;
inline constexpr uint8_t FANCTLENCLOSURE_PWM_MAX = 255;
inline constexpr uint16_t FANCTLENCLOSURE_RPM_MIN = 600;
inline constexpr uint16_t FANCTLENCLOSURE_RPM_MAX = 2700;
#endif // XL_ENCLOSURE_SUPPORT
