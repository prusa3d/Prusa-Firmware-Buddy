#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.h"

void GcodeSuite::M601() {
    marlin_server_print_pause();
}

void GcodeSuite::M602() {
    marlin_server_print_resume();
}
