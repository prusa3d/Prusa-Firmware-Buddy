/**
 * @file screen_menu_tune.cpp
 */

#include "screen_menu_tune.hpp"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "utility_extensions.hpp"

ScreenMenuTune::ScreenMenuTune()
    : ScreenMenuTune__(_(label)) {
    ScreenMenuTune__::ClrMenuTimeoutClose();
}

void ScreenMenuTune::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::LOOP:
        if (marlin_server::all_axes_homed()
            && marlin_server::all_axes_known()
            && (marlin_client::get_command() != marlin_server::Cmd::G28)
            && (marlin_client::get_command() != marlin_server::Cmd::G29)
            && (marlin_client::get_command() != marlin_server::Cmd::M109)
            && (marlin_client::get_command() != marlin_server::Cmd::M190)) {
            Item<MI_M600>().Enable();
        } else {
            Item<MI_M600>().Disable();
        }

#if ENABLED(CANCEL_OBJECTS)
        // Enable cancel object menu
        if (marlin_vars()->cancel_object_count > 0) {
            Item<MI_CO_CANCEL_OBJECT>().Enable();
        } else {
            Item<MI_CO_CANCEL_OBJECT>().Disable();
        }
#endif /* ENABLED(CANCEL_OBJECTS) */
        break;
    default:
        break;
    }
    SuperWindowEvent(sender, event, param);
}
