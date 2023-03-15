/**
 * @file screen_menu_tune.cpp
 */

#include "screen_menu_tune.hpp"
#include "marlin_client.hpp"

ScreenMenuTune::ScreenMenuTune()
    : ScreenMenuTune__(_(label)) {
    ScreenMenuTune__::ClrMenuTimeoutClose();
}

void ScreenMenuTune::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::LOOP:
        if (marlin_all_axes_homed() && marlin_all_axes_known() && (marlin_command() != MARLIN_CMD_G28) && (marlin_command() != MARLIN_CMD_G29) && (marlin_command() != MARLIN_CMD_M109) && (marlin_command() != MARLIN_CMD_M190)) {
            Item<MI_M600>().Enable();
        } else {
            Item<MI_M600>().Disable();
        }
        break;
    default:
        break;
    }
    SuperWindowEvent(sender, event, param);
}
