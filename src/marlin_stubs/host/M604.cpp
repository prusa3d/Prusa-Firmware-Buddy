#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.hpp"

void GcodeSuite::M604() {
    marlin_server::print_abort();
}
