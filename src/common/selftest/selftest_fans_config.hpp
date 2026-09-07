#pragma once

#include <printers.h>
#include <option/has_switched_fan_test.h>
#include <option/has_cpu_fan.h>
#include <option/has_xl_can.h>
#include <common/printer_model.hpp>
#include <common/extended_printer_type.hpp>
#if HAS_CPU_FAN()
    #include <fanctl/CFanCtlCommonConsts.hpp>
#endif

namespace fan_selftest {

using FanRPM = uint16_t;

struct FanRPMRange {
    FanRPM rpm_min;
    FanRPM rpm_max;

    static consteval FanRPMRange nominal_with_percentual_tolerance(FanRPM nominal, uint8_t tolerance_percent) {
        const FanRPM tolerance_rpm = static_cast<FanRPM>(nominal / 100.0f * tolerance_percent);
        return FanRPMRange {
            .rpm_min = static_cast<FanRPM>(nominal - tolerance_rpm),
            .rpm_max = static_cast<FanRPM>(nominal + tolerance_rpm),
        };
    }
};

constexpr FanRPMRange benevolent_fan_range = { .rpm_min = 10, .rpm_max = 10000 };

#if PRINTER_IS_PRUSA_XL()
///@note Datasheet says 5900 +-10%, but that is without any fan shroud.
///  Blocked fan increases its RPMs over 7000.
///  With XL shroud the values can be 6200 - 6600 depending on fan shroud version.
constexpr FanRPMRange print_fan_range_xl = { .rpm_min = 5300, .rpm_max = 7000 };
/// XLS uses LDO D5015G08B05X71 blower: datasheet values 7100 RPM +/-10%,
/// but when loaded it is higher, empirically measured value: 8000 RPM +/- 15%
constexpr FanRPMRange print_fan_range_xls = { .rpm_min = 6800, .rpm_max = 9200 };
constexpr FanRPMRange print_low_fan_range = benevolent_fan_range;
constexpr FanRPMRange heatbreak_fan_range = { .rpm_min = 6500, .rpm_max = 9800 };
#elif PRINTER_IS_PRUSA_MK4()
// Datasheet values for MK4S (5700+-10%) & MK4 (5900+-10%)
// range needs to be relaxed due to difference in atmospheric pressure and altitude during selftest
constexpr FanRPMRange print_fan_range = { .rpm_min = 4800, .rpm_max = 6799 };
constexpr FanRPMRange print_low_fan_range = benevolent_fan_range;
constexpr FanRPMRange heatbreak_fan_range = { .rpm_min = 6800, .rpm_max = 8700 };
#elif PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
constexpr FanRPMRange print_fan_range = { .rpm_min = 4800, .rpm_max = 6799 };
constexpr FanRPMRange print_low_fan_range = benevolent_fan_range;
    #if HAS_INDX()
constexpr FanRPMRange heatbreak_fan_range = { .rpm_min = 12000, .rpm_max = 18000 };
// The dock fan is the same hardware as the C1 print fan, so reuse its range.
constexpr FanRPMRange dock_fan_range = print_fan_range;
    #else
constexpr FanRPMRange heatbreak_fan_range = { .rpm_min = 6800, .rpm_max = 8700 };
    #endif
constexpr FanRPMRange chamber_fan_range = FanRPMRange::nominal_with_percentual_tolerance(8500, 15);
constexpr FanRPMRange filtration_fan_range = FanRPMRange::nominal_with_percentual_tolerance(3400, 15);
#elif PRINTER_IS_PRUSA_iX()
constexpr FanRPMRange print_fan_range = benevolent_fan_range;
constexpr FanRPMRange print_low_fan_range = benevolent_fan_range;
constexpr FanRPMRange heatbreak_fan_range = { .rpm_min = 1000, .rpm_max = 25000 };
#else
constexpr FanRPMRange print_fan_range = benevolent_fan_range;
constexpr FanRPMRange print_low_fan_range = benevolent_fan_range;
constexpr FanRPMRange heatbreak_fan_range = benevolent_fan_range;
#endif

#if HAS_SWITCHED_FAN_TEST() && !PRINTER_IS_PRUSA_MK3_5()
// MK3.5 has benevolent fan range - rendering switch fan test obsolete
static_assert(print_fan_range.rpm_max < heatbreak_fan_range.rpm_min, "These cannot overlap for switched fan detection.");
#endif

#if HAS_CPU_FAN()
/// CPU cooling fan on the XLS sandwich board. Range derives from
/// FANCTLCPU_RPM_MIN/MAX in CFanCtlCommonConsts.hpp (JDL3006S, nominal
/// 9000 RPM; see there for why the window is not the datasheet tolerance).
constexpr FanRPMRange cpu_fan_range = { .rpm_min = FANCTLCPU_RPM_MIN, .rpm_max = FANCTLCPU_RPM_MAX };
#endif

#if HAS_XL_CAN()
/// XLS Modular Bed cooling fan on the XL-CAN bridge. No nominal RPM has been
/// captured for this fan yet, so the test only verifies the fan spins and the
/// tacho reports
constexpr FanRPMRange bed_mcu_fan_range = benevolent_fan_range;
#endif

#if HAS_CPU_FAN() || HAS_XL_CAN()
/// Whether this machine carries the two board-cooling fans above. Their capability
/// macros are master-board-level and shared with plain XL, where neither fan exists.
/// The printer model is the discriminator rather than the runtime XL-CAN bridge
/// state, so that a stored selftest result is read back under the same condition
/// that decided to produce it.
[[nodiscard]] inline bool has_board_fans() {
    static_assert(PRINTER_IS_PRUSA_XL());
    static_assert(extended_printer_type_model == std::array { PrinterModel::xl, PrinterModel::xls },
        "New XL variant - decide whether it carries the CPU and Modular Bed MCU fans");
    return PrinterModelInfo::current().model == PrinterModel::xls;
}
#endif

} // namespace fan_selftest
