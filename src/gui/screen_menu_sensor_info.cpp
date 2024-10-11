#include "screen_menu_sensor_info.hpp"

#include <gui/dialogs/DialogMoveZ.hpp>

ScreenMenuSensorInfo::ScreenMenuSensorInfo()
    : ScreenMenuSensorInfo_(_("SENSOR INFO")) //
{
    EnableLongHoldScreenAction();
    ClrMenuTimeoutClose();
}

void ScreenMenuSensorInfo::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}
