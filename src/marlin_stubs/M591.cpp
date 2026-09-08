/// @file
#include "PrusaGcodeSuite.hpp"
#include <optional>
#include <feature/prusa/e-stall_detector.h>
#include <Marlin/src/module/planner.h>
#include <option/has_loadcell.h>
#include <config_store/store_instance.hpp>
#include <logging/log.hpp>

LOG_COMPONENT_REF(MarlinServer);

namespace {
enum class IsPermanent { no,
    yes };
enum class Restore { no,
    yes };

#if HAS_LOADCELL()
void m591_no_parser(std::optional<bool> opt_enable_e_stall, IsPermanent is_permanent, Restore restore, std::optional<std::tuple<uint32_t, uint32_t>> ignore) {
    // wait for all previously queued moves to finish to only apply the new settings for moves that follow
    planner.synchronize();

    if (restore == Restore::yes) {
        bool enabled_in_eeprom = config_store().stuck_filament_detection.get();
        EMotorStallDetector::Instance().SetEnabled(enabled_in_eeprom);
        log_info(MarlinServer, "E-stall detection %s (restore)", enabled_in_eeprom ? "on" : "off");
        return; // ignore remaining parameters
    }

    if (opt_enable_e_stall) {
        bool enable_e_stall = *opt_enable_e_stall;

        // write to eeprom
        if (is_permanent == IsPermanent::yes) {
            config_store().stuck_filament_detection.set(enable_e_stall);
        }

        EMotorStallDetector::Instance().SetEnabled(enable_e_stall);
        log_info(MarlinServer, "E-stall detection %s%s", enable_e_stall ? "on" : "off", is_permanent == IsPermanent::yes ? " permanent" : "");

    } else {
        // restore and opt_enable_e_stall parameters not supplied
        SERIAL_ECHOLNPAIR_F("Filament stuck detection ", EMotorStallDetector::Instance().Enabled() ? "on" : "off");
    }

    if (ignore.has_value()) {
        auto [count, forget] = ignore.value();
        EMotorStallDetector::Instance().SetIgnore(count, forget);
    }
}
#endif // HAS_LOADCELL()
} // anonymous namespace

/** \addtogroup G-Codes
 * @{
 */

/**### M591: Enable/Disable Filament stuck monitoring <a href="https://reprap.org/wiki/G-code#M591:_Configure_filament_monitoring">M591: Configure filament monitoring</a>
 *
 * Only MK3.9/S, MK4/S, iX and XL
 *
 *#### Usage
 *
 *    M591 [ S | P | R ]
 *
 *#### Parameters
 *
 * - `S` - Enable / Disable
 * - `P` - change is permanent
 * - `R` - restore, this parameter has priority over `S` and `P` and discards them
 * - `I` - ignore this many skips before reporting (0 = report immediatelly, 1 - report the second if within the forget time)
 * - `F` - forget a skip after this time (in combination with I, in ms)
 *   Set both at once.
 *
 *   TODO: Can one parameter take two numbers?
 *
 * Without parameters prints the current state of Filament stuck monitoring (on/off)
 *
 * After the change or in case the `S` parameter is missing, it prints the state of EMotorStallDetector onto the serial line
 */

void PrusaGcodeSuite::M591() {
#if HAS_LOADCELL()
    std::optional<bool> enable_e_stall;
    if (parser.seen('S')) {
        enable_e_stall = parser.byteval('S') == 1;
    }
    IsPermanent is_permanent = parser.seen('P') ? IsPermanent::yes : IsPermanent::no;
    Restore restore = parser.seen('R') ? Restore::yes : Restore::no;
    std::optional<std::tuple<uint32_t, uint32_t>> ignore = std::nullopt;
    if (parser.seen('I') && parser.seen('F')) {
        ignore = std::make_tuple(parser.ulongval('I'), parser.ulongval('F'));
    }
    m591_no_parser(enable_e_stall, is_permanent, restore, ignore);
#else
    SERIAL_ECHOLN("Filament stuck detection not supported");
#endif // HAS_LOADCELL()
}

/** @}*/
