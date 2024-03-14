#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.hpp"

#include <option/has_leds.h>
#if HAS_LEDS()
    #include "led_animations/printer_animation_state.hpp"
#endif

void GcodeSuite::M601() {
    marlin_server::print_pause();
#if HAS_LEDS()
    PrinterStateAnimation::force_printer_state_until(PrinterState::Warning, PrinterState::Printing);
#endif
}

void GcodeSuite::M602() {
    marlin_server::print_resume();
}
