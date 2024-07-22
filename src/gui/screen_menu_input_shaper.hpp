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
    MI_IS_CALIB_RESULT_UNKNOWN,
    MI_IS_CALIB_RESULT_SKIPPED,
    MI_IS_CALIB_RESULT_PASSED,
    MI_IS_CALIB_RESULT_FAILED,
#endif
    MI_IS_ENABLE_EDITING,
    MI_IS_X_ONOFF,
    MI_IS_X_TYPE,
    MI_IS_X_FREQUENCY,
    MI_IS_Y_ONOFF,
    MI_IS_Y_TYPE,
    MI_IS_Y_FREQUENCY,
    MI_IS_Y_COMPENSATION,
    MI_IS_RESTORE_DEFAULTS>;
} // namespace detail

class ScreenMenuInputShaper : public detail::ScreenMenuInputShaper {

public:
    constexpr static const char *label = N_("INPUT SHAPER");
    ScreenMenuInputShaper();

    /// Updates values & states of the menu items to match the current IS config
    void update_gui();

public:
    bool is_editing_enabled = false; // Is set to true in the constructor if MI_IS_ENABLE_EDITING is hidden

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param);

private:
    bool is_updating_gui = false;
};
