#include "print_utils.h"
#include <stdint.h>
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "marlin_client.h"

extern "C" void print_begin(const char *filename) {
    marlin_print_start(filename);
    // FIXME: This should not be here and it should be handled
    // in Marlin. Needs refactoring!
    oProgressData.mInit();
}
