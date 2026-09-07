#include "fanctl.hpp"
#include "bsod.h"
#include "Marlin/src/inc/MarlinConfig.h" // HOTENDS
#include <array>
#include "CFanCtlEnclosure.hpp"
#include "CFanCtlPuppy.hpp"
#include "hwio_pindef.h"
#include <option/has_cpu_fan.h>
#if HAS_CPU_FAN()
    #include <CFanCtlCommonConsts.hpp>
    #include <fanctl/CFanCtl3Wire.hpp>
#endif

CFanCtlCommon &Fans::print(PhysicalToolIndex tool) {
    static std::array<CFanCtlPuppy, PhysicalToolIndex::count> instances = {
        CFanCtlPuppy(0, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(1, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(2, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(3, 0, false, FANCTLPRINT_RPM_MAX),
        CFanCtlPuppy(4, 0, false, FANCTLPRINT_RPM_MAX),
    };
    return instances[tool.to_raw()];
}
CFanCtlCommon &Fans::heat_break(PhysicalToolIndex tool) {
    static std::array<CFanCtlPuppy, PhysicalToolIndex::count> instances = {
        CFanCtlPuppy(0, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(1, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(2, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(3, 1, true, FANCTLHEATBREAK_RPM_MAX),
        CFanCtlPuppy(4, 1, true, FANCTLHEATBREAK_RPM_MAX),
    };
    return instances[tool.to_raw()];
}

CFanCtlCommon &Fans::enclosure() {
    static auto instance = CFanCtlEnclosure(
        buddy::hw::fan1_tach0,
        FANCTLENCLOSURE_RPM_MIN, FANCTLENCLOSURE_RPM_MAX);

    return instance;
};

#if HAS_CPU_FAN()
CFanCtlCommon &Fans::cpu() {
    static auto instance = CFanCtl3Wire(
        [](bool value) { buddy::hw::cpuFanPwm.writeb(value); },
        []() { return buddy::hw::cpuFanTach.readb(); },
        {
            .min_pwm = FANCTLCPU_PWM_MIN,
            .max_pwm = FANCTLCPU_PWM_MAX,
            .min_rpm = FANCTLCPU_RPM_MIN,
            .max_rpm = FANCTLCPU_RPM_MAX,
            .thr_pwm = FANCTLCPU_PWM_THR,
            .autofan = is_autofan_t::no,
            .skip_tacho = skip_tacho_t::no,
            .min_pwm_to_measure_rpm = FANCTLCPU_MIN_PWM_TO_MEASURE_RPM,
            .has_inverted_pwm = false,
        });
    return instance;
}
#endif

void Fans::tick() {
    Fans::enclosure().tick();
#if HAS_CPU_FAN()
    Fans::cpu().tick();
#endif
}

void Fans::init_hw() {
    buddy::hw::fanPowerSwitch.set();
}
