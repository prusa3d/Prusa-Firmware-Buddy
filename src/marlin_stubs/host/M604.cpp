#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.hpp"

/**
 * M604 - Abort (serial) print
 *
 * This is expected to be set as end-print command in octoprint
 */
void GcodeSuite::M604() {
    marlin_server::print_abort();
}
