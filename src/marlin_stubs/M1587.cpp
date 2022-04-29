#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include "selftest_esp.hpp"

/**
 * M1587: Open Wi-Fi credentials dialog
 * Similar to M587, but meat to be used internally
 */
void PrusaGcodeSuite::M1587() {
    update_esp_credentials();
}
