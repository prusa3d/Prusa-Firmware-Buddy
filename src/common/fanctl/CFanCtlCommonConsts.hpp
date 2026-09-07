#pragma once

#include <printers.h>
#include <option/xl_enclosure_support.h>
#include <option/has_cpu_fan.h>

// FANCTLPRINT - printing fan
inline constexpr uint8_t FANCTLPRINT_PWM_MIN = 10; // min duty cycle length 10 / 50 = 0.2 = 20%
/// Min PWM for XLS native mode (LDO blower with kickstart): 3 / 50 = 6%.
/// Applied at runtime on the Dwarf when the XLBuddy sets fan_mode != 0.
///
/// Kickstart spins the fan up past its stall torque so the steady-state PWM
/// can sit below the 20% threshold required by the legacy Delta/GOM fans.
///
/// Lower bound tuned empirically against the tach sampler in
/// CFanCtlTach::tick(), which only counts a tach transition as an edge when
/// pwm_on >= 2. Inside an on-window pwm_on ranges 0..val-1, so:
///   - val = 2 (4%): no on-tick ever reaches pwm_on = 2 -> edges = 0,
///     reported RPM = 0, Sensor Info shows "stuck" even though the fan is
///     clearly spinning. Observed on bench.
///   - val = 3 (6%): 1 of 3 on-ticks qualifies. Reported RPM is noisy but
///     consistently non-zero (observed 16..680 RPM on the LDO blower with
///     kickstart at 3/50 PWM). Combined with FANCTLPRINT_RPM_MIN = 10 on
///     XL, this is the lowest usable PWM that still produces a measurable
///     and non-erroring tach reading.
inline constexpr uint8_t FANCTLPRINT_PWM_MIN_XLS = 3;
inline constexpr uint8_t FANCTLPRINT_PWM_MAX = 50; // 1000Hz / 50 = 20Hz PWM cycle
#if PRINTER_IS_PRUSA_MK4()
inline constexpr uint16_t FANCTLPRINT_RPM_MIN = 90; // Dynamic PWM enables lower RPM
#elif PRINTER_IS_PRUSA_XL()
/// XL/XLS share Dwarf firmware. XLS LDO blower at 3/50 PWM bottoms out around
/// 16 RPM on the tach (see FANCTLPRINT_PWM_MIN_XLS). XL legacy Delta/GOM fans
/// cruise at 5300-7000 RPM during print, so 10 has ample margin for them too.
/// ErrorChecker::checkTrue has no debounce — any single sample below this
/// trips PrintFanError — so the threshold must stay below the worst-case
/// EWMA dip at the lowest supported PWM, not just the typical value.
inline constexpr uint16_t FANCTLPRINT_RPM_MIN = 10;
#else
inline constexpr uint16_t FANCTLPRINT_RPM_MIN = 150;
#endif
inline constexpr uint16_t FANCTLPRINT_RPM_MAX =
#if HAS_INDX()
    10000
#elif (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL())
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
#if HAS_INDX()
    20000
#elif (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL())
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

// FANCTLCPU - CPU cooling fan (XLS sandwich board)
// Driver-level thresholds used by CFanCtl3Wire / get_rpm_is_ok().
// Selftest pass/fail tolerances live separately in selftest_fans_config.hpp.
//
// Fan: JDL3006S, 5 V 3-wire (VCC/GND/FG, no PWM input), 4-pole motor, rated
// 9000 RPM ±20 %, operating range 4.5-5.5 V (datasheet: BFW-8968). PWM drives
// a MOSFET on the sandwich board's 5 V supply. The cpu_fan_controller runs it at
// 0 % or 100 % only -- at 100 % duty the CFanCtlPWM output pin stays
// permanently high, so the 20 Hz soft-PWM frequency is not audible in
// practice.
//
// The window below is not the datasheet tolerance: MAX is the +15 % corner,
// while MIN sits well under the -20 % corner because the fans read
// noticeably slower mounted on the machine than they do free-air.
//
// XLS_TODO: the datasheet ticks only RED(+)/BLACK(-) ("2 Wires") and leaves
// "Tachometer Output" unchecked. That is a datasheet error -- the parts we
// have are physically 3-wire and RPM reads correctly, so cpuFanTach and the
// RPM selftest are sound. Awaiting Product's confirmation that the shipped
// part is consistently the 3-wire build.
#if HAS_CPU_FAN()
inline constexpr uint8_t FANCTLCPU_PWM_MIN = 20;
inline constexpr uint8_t FANCTLCPU_PWM_MAX = 100;
inline constexpr uint8_t FANCTLCPU_PWM_THR = 90; // 4.5V is the bottom of the fan's operating range, which is 90% of 5V
inline constexpr uint8_t FANCTLCPU_MIN_PWM_TO_MEASURE_RPM = 0;
// TODO BFW-9269 - limits relaxed due to the problems reported on production line. To be fine tuned later
inline constexpr uint16_t FANCTLCPU_RPM_MIN = 3000;
inline constexpr uint16_t FANCTLCPU_RPM_MAX = 15000;
#endif // HAS_CPU_FAN
