/**
 * @file screen_menu_user_interface.cpp
 */

#include "screen_menu_user_interface.hpp"
#include "gcode_info.hpp"
#include "DialogMoveZ.hpp"

void ScreenMenuUserInterfaceInSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenMenuUserInterfaceInSettings::ScreenMenuUserInterfaceInSettings()
    : ScreenMenuUserInterfaceInSettings__(_(label)) {
    EnableLongHoldScreenAction();
}

ScreenMenuUserInterfaceInTune::ScreenMenuUserInterfaceInTune()
    : ScreenMenuUserInterfaceInSettings() {
    EnableLongHoldScreenAction();
    GCodeInfo::getInstance().initFile(GCodeInfo::GI_INIT_t::PRINT);
    if (!GCodeInfo::getInstance().has_progress_thumbnail) {
        Hide<MI_PRINT_PROGRESS_TIME>();
    }
}
