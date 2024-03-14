#pragma once

#include <stdint.h>
#include <printers.h>

class CfanCtlCommon {
public:
    enum FanState : int8_t {
        idle = 0, // idle - no rotation, PWM = 0%
        starting = 1, // starting - PWM = 100%, waiting for 4 tacho edges
        rpm_stabilization = 2, // tick delay for reaching wanted rpm
        running = 3, // running - PWM set by setPWM(), no regulation
        error_starting = -1, // starting error - means no feedback after timeout expired
        error_running = -2, // running error - means zero RPM measured (no feedback)
    };
};

extern void record_fanctl_metrics();

// FANCTLPRINT - printing fan
inline constexpr uint8_t FANCTLPRINT_PWM_MIN = 10;
inline constexpr uint8_t FANCTLPRINT_PWM_MAX =
#if PRINTER_IS_PRUSA_MK3_5
    255
#else
    50
#endif
    ;
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
inline constexpr uint8_t FANCTLHEATBREAK_PWM_MAX =
#if PRINTER_IS_PRUSA_MK3_5
    255
#else
    50
#endif
    ;
inline constexpr uint16_t FANCTLHEATBREAK_RPM_MIN = 1000;
inline constexpr uint16_t FANCTLHEATBREAK_RPM_MAX =
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX || PRINTER_IS_PRUSA_XL)
    15180
#else
    8000
#endif
    ;
inline constexpr uint8_t FANCTLHEATBREAK_PWM_THR = 20;
