/**
 * @file power_check_both.hpp
 * @author Radek Vana
 * @brief Checks power consumption during selftest heater check
 * calculates with both heaters bed and nozzle
 * @date 2021-11-12
 */

#include "power_check.hpp"
#include "selftest_heaters_type.hpp"

namespace selftest {
class PowerCheckBoth {
    PowerCheck noz;
    PowerCheck bed;

    LogTimer log_noz;
    LogTimer log_bed;

    constexpr PowerCheckBoth()
        : log_noz(3000)
        , log_bed(3000) {}
    PowerCheckBoth(const PowerCheckBoth &) = delete;

public:
    void Callback();

    constexpr void BindNozzle(CSelftestPart_Heater &f) {
        noz.Bind(f);
    }
    constexpr void BindBed(CSelftestPart_Heater &f) {
        bed.Bind(f);
    }
    constexpr void UnBindNozzle() { noz.UnBind(); }
    constexpr void UnBindBed() { bed.UnBind(); }

    static PowerCheckBoth &Instance() {
        static PowerCheckBoth ret;
        return ret;
    }
};
} // namespace selftest
