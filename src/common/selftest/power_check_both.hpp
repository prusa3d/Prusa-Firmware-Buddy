/**
 * @file power_check_both.hpp
 * @author Radek Vana
 * @brief Checks power consumption during selftest heater check
 * calculates with both heaters bed and nozzle
 * @date 2021-11-12
 */

#include "power_check.hpp"
#include "selftest_heaters_type.hpp"
#include "selftest_heater.h"

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
    void Callback(CSelftestPart_Heater &part);

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
