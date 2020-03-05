#include "print_utils.h"
#include <stdint.h>
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "marlin_client.h"
#include "screen_printing.h"

extern "C" void print_begin(const char *filename) {
    // set gcode name into marlin_vars_t for WUI
    marlin_set_printing_gcode_name(screen_printing_file_name);
    marlin_gcode_printf("M23 %s", filename);
    marlin_gcode("M24");
    // FIXME: This should not be here and it should be handled
    // in Marlin. Needs refactoring!
    oProgressData.mInit();
}
