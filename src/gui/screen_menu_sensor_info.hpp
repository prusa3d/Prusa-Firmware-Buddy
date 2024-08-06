/**
 * @file screen_menu_sensor_info.hpp
 */
#pragma once

#include "screen_menu_sensor_info_parent_alias.hpp"
#include "fs_autoload_autolock.hpp"

class ScreenMenuSensorInfo : public detail::ScreenMenuSensorInfo {
    FS_AutoloadAutolock lock;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("SENSOR INFO");
    ScreenMenuSensorInfo()
        : detail::ScreenMenuSensorInfo(_(label)) {
        EnableLongHoldScreenAction();
        ClrMenuTimeoutClose();
    }
};
