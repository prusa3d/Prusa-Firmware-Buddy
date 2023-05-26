/**
 * @file screen_menu_input_shaper.hpp
 * @brief displaying and setting of input shaper values
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"

namespace detail {
using ScreenMenuInputShaper = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_IS_ONOFF, MI_IS_X_TYPE, MI_IS_X_FREQUENCY, MI_IS_Y_TYPE, MI_IS_Y_FREQUENCY>;
}

class ScreenMenuInputShaper : public detail::ScreenMenuInputShaper {
public:
    constexpr static const char *label = N_("INPUT SHAPER (ALPHA)");
    ScreenMenuInputShaper();
};
