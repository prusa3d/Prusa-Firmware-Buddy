#include "window_lcd_message.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"

#include "M117.hpp"
#include <stdint.h>

/**
 * M117: LCD message
 */
void PrusaGcodeSuite::M117() {
    if (parser.string_arg && parser.string_arg[0])
        strncpy(lcd_message_text, parser.string_arg, LCD_MESSAGE_MAX_LEN);
    else
        lcd_message_text[0] = 0;
}
