/// @file
#include "PrusaGcodeSuite.hpp"
#include <optional>
#include <feature/prusa/e-stall_detector.h>
#include <option/has_loadcell.h>
#include <config_store/store_instance.hpp>

LOG_COMPONENT_REF(MarlinServer);

namespace {
enum class IsPermanent { no,
    yes };
enum class Restore { no,
    yes };

#if HAS_LOADCELL()
void m591_no_parser(std::optional<bool> opt_enable_e_stall, IsPermanent is_permanent, Restore restore) {
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
        log_info(MarlinServer, "E-stall detection %s%s", enable_e_stall ? "on" : "off", is_permanent == IsPermanent::yes ? " permanent" : nullptr);

    } else {
        // restore and opt_enable_e_stall parameters not supplied
        SERIAL_ECHOLNPAIR_F("Filament stuck detection ", EMotorStallDetector::Instance().Enabled() ? "on" : "off");
    }
}
#endif // HAS_LOADCELL()
} // anonymous namespace

/** \addtogroup G-Codes
 * @{
 */

/// @brief Enable/Disable Filament stuck monitoring
/// Prusa STM32 platform specific
/// ## Parameters
/// - `S` - 0 disable
///       - 1 enable
/// - `P` - change is permanent (saved in EEPROM)
/// - `R` - restore, this parameter has priority over `S` and `P` and discards them
///
/// - without parameters prints the current state of Filament stuck monitoring (on/off)
///
/// - After the change or in case the `S` parameter is missing, it prints the state of EMotorStallDetector onto the serial line

void PrusaGcodeSuite::M591() {
#if HAS_LOADCELL()
    std::optional<bool> enable_e_stall;
    if (parser.seen('S')) {
        enable_e_stall = parser.byteval('S') == 1;
    }
    IsPermanent is_permanent = parser.seen('P') ? IsPermanent::yes : IsPermanent::no;
    Restore restore = parser.seen('R') ? Restore::yes : Restore::no;
    m591_no_parser(enable_e_stall, is_permanent, restore);
#else
    SERIAL_ECHOLN("Filament stuck detection not supported");
#endif // HAS_LOADCELL()
}

/** @}*/
