/**
 * @file screen_menu_sensor_info.hpp
 */
#pragma once

#include "screen_menu_sensor_info_parent_alias.hpp"
#include "fs_autoload_autolock.hpp"
#include "../../common/sensor_data_buffer.hpp"

class ScreenMenuSensorInfo : public detail::ScreenMenuSensorInfo {
    FS_AutoloadAutolock lock;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("SENSOR INFO");
    SensorData::SensorDataBuffer buffer;
    ScreenMenuSensorInfo()
        : detail::ScreenMenuSensorInfo(_(label)) {
        EnableLongHoldScreenAction();
        ClrMenuTimeoutClose();
    }
};
