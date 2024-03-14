/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "gcode_info.hpp"

#ifdef PRINT_CHECKING_Q_CMDS
namespace PrusaGcodeSuite {
M862_6SupportedFeatures m862_6SupportedFeatures = { "Input shaper" };
}

/**
 * M862.6: Check gcode level
 */
void PrusaGcodeSuite::M862_6() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (parser.boolval('Q')) {
        char temp_buf[sizeof("  M862.6 P\"01234567890123456789\"")];
        for (auto &feature : m862_6SupportedFeatures) {
            SERIAL_ECHO_START();
            snprintf(temp_buf, sizeof(temp_buf), PSTR("  M862.6 P\"%s\""), feature);
            SERIAL_ECHO(temp_buf);
            SERIAL_EOL();
        }
    }
}
#endif
