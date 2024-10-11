#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.hpp"

#include <option/has_leds.h>
#if HAS_LEDS()
    #include "led_animations/printer_animation_state.hpp"
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M601: Pause print <a href="https://reprap.org/wiki/G-code#M601:_Pause_print">M601: Pause print</a>
 *
 *#### Usage
 *
 *    M601
 */

void GcodeSuite::M601() {
    marlin_server::print_pause();
#if HAS_LEDS()
    PrinterStateAnimation::force_printer_state_until(PrinterState::Warning, PrinterState::Printing);
#endif
}

/**
 *### M602: Resume print <a href="https://reprap.org/wiki/G-code#M602:_Resume_print">M602: Resume print</a>
 *
 *#### Usage
 *
 *    M602
 */

void GcodeSuite::M602() {
    marlin_server::print_resume();
}

/** @}*/
