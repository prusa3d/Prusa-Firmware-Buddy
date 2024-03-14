/**
 * @file screen_menu_input_shaper.hpp
 * @brief displaying and setting of input shaper values
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"

namespace detail {
using ScreenMenuInputShaper = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_IS_SET, MI_IS_X_ONOFF, MI_IS_X_TYPE, MI_IS_X_FREQUENCY, MI_IS_Y_ONOFF, MI_IS_Y_TYPE, MI_IS_Y_FREQUENCY, MI_IS_Y_COMPENSATION /*, MI_IS_CALIB*/>;
}

class ScreenMenuInputShaper : public detail::ScreenMenuInputShaper {
public:
    constexpr static const char *label = N_("INPUT SHAPER");
    ScreenMenuInputShaper();

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
};
