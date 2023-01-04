/**
 * @file screen_menu_calibration.hpp
 */
#pragma once

#include "screen_menu_calibration_parent_alias.hpp"

class ScreenMenuCalibration : public ScreenMenuCalibration__ {
public:
    constexpr static const char *label = N_("CALIBRATION");
    ScreenMenuCalibration();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
