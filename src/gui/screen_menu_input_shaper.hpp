/**
 * @file screen_menu_input_shaper.hpp
 * @brief displaying and setting of input shaper values
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_input_shaper.hpp"
#include <option/has_input_shaper_calibration.h>

namespace detail {

using ScreenMenuInputShaper = ScreenMenu<
    GuiDefaults::MenuFooter,
    MI_RETURN,
#if HAS_INPUT_SHAPER_CALIBRATION()
    MI_IS_CALIB,
#endif
    MI_IS_X_TYPE,
    MI_IS_X_FREQUENCY,
    MI_IS_Y_TYPE,
    MI_IS_Y_FREQUENCY,
    MI_IS_RESTORE_DEFAULTS>;
} // namespace detail

class ScreenMenuInputShaper : public detail::ScreenMenuInputShaper {

public:
    constexpr static const char *label = N_("INPUT SHAPER");
    ScreenMenuInputShaper();

    /// Updates values & states of the menu items to match the current IS config
    void update_gui();

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param);

private:
    bool is_updating_gui = false;
};
