#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include "selftest_esp.hpp"

/**
 * M1587: Open Wi-Fi credentials dialog
 * Similar to M587, but meat to be used internally
 *  I          - Generate ini file
 */
void PrusaGcodeSuite::M1587() {
    if (parser.seen('I')) {
        credentials_generate_ini();
    } else {
        update_esp_credentials();
    }
}
