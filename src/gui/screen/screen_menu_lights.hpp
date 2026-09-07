/// @file
#pragma once

#include "MItem_tools.hpp"
#include <basic_screen_menu.hpp>
#include <option/has_leds.h>
#include <option/has_side_leds.h>
#include <option/has_toolchanger.h>

using ScreenMenuLightsBase = BasicScreenMenu<
#if HAS_LEDS()
    MI_LEDS_ENABLE,
#endif
#if HAS_TOOLCHANGER()
    MI_TOOL_LEDS_ENABLE,
#endif
#if HAS_SIDE_LEDS()
    MI_SIDE_LEDS_MAX_BRIGTHNESS,
    MI_SIDE_LEDS_DIMMING_ENABLE,
    MI_SIDE_LEDS_DIMMED_BRIGTHNESS,
#endif
    MI_ALWAYS_HIDDEN>;

class ScreenMenuLights final : public ScreenMenuLightsBase {
public:
    ScreenMenuLights();
};
