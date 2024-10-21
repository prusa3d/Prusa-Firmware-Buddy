#pragma once

#include "power_check.hpp"
#include "selftest_heaters_type.hpp"
#include "selftest_heater.h"
#include "selftest_heater_config.hpp"

#if !HAS_SELFTEST_POWER_CHECK_BOTH()
    #error This should have not been included
#endif

namespace selftest {
// This singleton object is used to check power of linked heaters
//
// TODO: This needs to be checked for MK4 as the code was modifed with the MK4
// power check disabled.
class PowerCheckBoth {
    CSelftestPart_Heater *nozzle;
    CSelftestPart_Heater *bed;

    PowerCheckBoth() = default;
    PowerCheckBoth(const PowerCheckBoth &) = delete;

public:
    void Callback([[maybe_unused]] CSelftestPart_Heater &part);

    constexpr void BindNozzle(CSelftestPart_Heater *part) {
        nozzle = part;
    }
    constexpr void BindBed(CSelftestPart_Heater *part) {
        bed = part;
    }
    constexpr void UnBindNozzle() { nozzle = nullptr; }
    constexpr void UnBindBed() { bed = nullptr; }

    static PowerCheckBoth &Instance() {
        static PowerCheckBoth ret;
        return ret;
    }
};
} // namespace selftest
