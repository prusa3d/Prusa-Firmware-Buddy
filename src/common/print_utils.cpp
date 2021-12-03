#include "print_utils.hpp"
#include <stdint.h>
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "../lib/Marlin/Marlin/src/module/temperature.h"
#include "marlin_client.h"
#include "media.h"
#include "timing.h"
#include "marlin_server.hpp"
#include "guiconfig.h" // GUI_WINDOW_SUPPORT
#include "unistd.h"

#if AUTOSTART_GCODE
static const char *autostart_filename = "/usb/AUTO.GCO";
#endif
static bool run_once_done = false;
static uint32_t current_time = 0;
static uint32_t rescan_delay = 1500;
static uint32_t max_rescan_time = 10000;

void run_once_after_boot() {
    // g-code autostart
#if AUTOSTART_GCODE
    if (access(autostart_filename, F_OK) == 0) {
        //call directly marlin server start print. This function is not safe
        marlin_server_print_start(autostart_filename);
        oProgressData.mInit();
    }
#endif
}

void print_utils_loop() {
    if (run_once_done == false && HAL_GetTick() >= current_time + rescan_delay) {
        current_time += rescan_delay;
        if (media_get_state() == media_state_INSERTED) {
            run_once_done = true;
            run_once_after_boot();
        } else if (current_time > max_rescan_time || !marlin_server_printer_idle()) {
            // no longer attempt to run the autostart sequence
            run_once_done = true;
        }
    }
}

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
