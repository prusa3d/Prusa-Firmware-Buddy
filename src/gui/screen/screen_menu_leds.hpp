/**
 * @file screen_menu_leds.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include <option/has_toolchanger.h>

using ScreenMenuLeds__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
#if HAS_LEDS()
    MI_LEDS_ENABLE,
    MI_DISPLAY_BACKLIGHT_BRIGHTNESS,
#endif
#if HAS_TOOLCHANGER()
    MI_TOOL_LEDS_ENABLE,
    MI_TOOL_LEDS_BRIGHTNESS,
#endif
#if HAS_SIDE_LEDS()
    MI_SIDE_LEDS_MAX_BRIGTHNESS,
    MI_SIDE_LEDS_DIMMING_ENABLE,
    MI_SIDE_LEDS_DIMMING_DURATION,
    MI_SIDE_LEDS_DIMMED_BRIGTHNESS,
#endif
    MI_ALWAYS_HIDDEN>;

class ScreenMenuLeds : public ScreenMenuLeds__ {
public:
    ScreenMenuLeds();
};
