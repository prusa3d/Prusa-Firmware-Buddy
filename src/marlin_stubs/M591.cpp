/// @file
#include "PrusaGcodeSuite.hpp"
#include <feature/prusa/e-stall_detector.h>
#include <option/has_loadcell.h>

/// @brief Enable/Disable Filament stuck monitoring
/// Prusa STM32 platform specific
/// ## Parameters
/// - `S` - 0 disable
///       - 1 enable
/// - After the change or in case the `S` parameter is missing, it prints the state of EMotorStallDetector onto the serial line
void PrusaGcodeSuite::M591() {
#if HAS_LOADCELL()
    if (parser.seen('S')) {
        if (parser.byteval('S') == 1) {
            EMotorStallDetector::Instance().Enable();
        } else {
            EMotorStallDetector::Instance().Enable();
        }
    }
    SERIAL_ECHOLNPAIR_F("Filament stuck detection ", EMotorStallDetector::Instance().Enabled() ? "enabled" : "disabled");
#else
    SERIAL_ECHOLN("Filament stuck detection not supported");
#endif
}
