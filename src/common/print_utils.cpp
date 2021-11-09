#include "print_utils.hpp"
#include <stdint.h>
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "marlin_client.h"
#include "timing.h"
#include "guiconfig.h" // GUI_WINDOW_SUPPORT

#ifdef GUI_WINDOW_SUPPORT
    #include "ScreenHandler.hpp"
    #include "screen_printing.hpp"

void print_begin(const char *filename) {
    Screens::Access()->CloseAll();
    marlin_print_start(filename);
    // FIXME: This should not be here and it should be handled
    // in Marlin. Needs refactoring!
    oProgressData.mInit();
    Screens::Access()->Open(ScreenFactory::Screen<screen_printing_data_t>);
}

#else  // GUI_WINDOW_SUPPORT

void print_begin(const char *filename) {
    marlin_print_start(filename);
    // FIXME: This should not be here and it should be handled
    // in Marlin. Needs refactoring!
    oProgressData.mInit();
}
#endif // GUI_WINDOW_SUPPORT
