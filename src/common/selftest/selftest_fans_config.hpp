#pragma once

#include <printers.h>

namespace fan_selftest {

struct FanRPMRange {
    uint16_t rpm_min;
    uint16_t rpm_max;
};

constexpr FanRPMRange benevolent_fan_range = { .rpm_min = 10, .rpm_max = 10000 };

#if PRINTER_IS_PRUSA_XL()
///@note Datasheet says 5900 +-10%, but that is without any fan shroud.
///  Blocked fan increases its RPMs over 7000.
///  With XL shroud the values can be 6200 - 6600 depending on fan shroud version.
constexpr FanRPMRange print_fan_range = { .rpm_min = 5300, .rpm_max = 7000 };
constexpr FanRPMRange heatbreak_fan_range = { .rpm_min = 6500, .rpm_max = 8700 };
#elif PRINTER_IS_PRUSA_MK4()
constexpr FanRPMRange print_fan_range = { .rpm_min = 5300, .rpm_max = 6799 };
constexpr FanRPMRange heatbreak_fan_range = { .rpm_min = 6800, .rpm_max = 8700 };
static_assert(print_fan_range.rpm_max < heatbreak_fan_range.rpm_min, "These cannot overlap for switched fan detection.");
#else
constexpr FanRPMRange print_fan_range = benevolent_fan_range;
constexpr FanRPMRange heatbreak_fan_range = benevolent_fan_range;
#endif

} // namespace fan_selftest
