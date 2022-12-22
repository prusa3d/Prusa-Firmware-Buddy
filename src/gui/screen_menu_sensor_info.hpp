/**
 * @file screen_menu_sensor_info.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "fs_autoload_autolock.hpp"

using ScreenMenuSensorInfo__ = ScreenMenu<EFooter::On, MI_RETURN, MI_FILAMENT_SENSOR_STATE, MI_MINDA>;

class ScreenMenuSensorInfo : public ScreenMenuSensorInfo__ {
    FS_AutoloadAutolock lock;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("SENSOR INFO");
    ScreenMenuSensorInfo();
};
