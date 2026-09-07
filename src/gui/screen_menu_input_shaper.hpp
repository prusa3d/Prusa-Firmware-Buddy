/// @file
#pragma once

#include <basic_screen_menu.hpp>
#include "MItem_input_shaper.hpp"
#include <option/has_input_shaper_calibration.h>

using ScreenMenuInputShaperBase = BasicScreenMenu<
#if HAS_INPUT_SHAPER_CALIBRATION()
    MI_IS_CALIB,
#endif
    MI_IS_X_TYPE,
    MI_IS_X_FREQUENCY,
    MI_IS_Y_TYPE,
    MI_IS_Y_FREQUENCY,
    MI_IS_RESTORE_DEFAULTS>;

class ScreenMenuInputShaper final : public ScreenMenuInputShaperBase {
public:
    ScreenMenuInputShaper();

    /// Updates values & states of the menu items to match the current IS config
    void update_gui();

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param);

private:
    bool is_updating_gui = false;
};
