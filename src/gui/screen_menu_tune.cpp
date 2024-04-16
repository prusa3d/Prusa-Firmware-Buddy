/**
 * @file screen_menu_tune.cpp
 */

#include "screen_menu_tune.hpp"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "utility_extensions.hpp"
#include <option/has_mmu2.h>
#if XL_ENCLOSURE_SUPPORT()
    #include "xl_enclosure.hpp"
#endif

ScreenMenuTune::ScreenMenuTune()
    : ScreenMenuTune__(_(label)) {
    ScreenMenuTune__::ClrMenuTimeoutClose();

#if HAS_MMU2()
    // Do not allow disabling filament sensor
    if (config_store().mmu2_enabled.get()) {
        Item<MI_FILAMENT_SENSOR>().hide();
    }
#endif
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

#if XL_ENCLOSURE_SUPPORT()
        /* Once is Enclosure enabled in menu with ON/OFF switch (MI_ENCLOSURE_ENABLED), it tests the fan and if it passes, Enclosure is declared Active */
        /* If the test passes, MI_ENCLOSURE_ENABLE is swapped with MI_ENCLOSURE and enclosure settings can be accessed */
        /* This hides enclosure settings for Users without enclosure */

        if (xl_enclosure.isActive() && Item<MI_ENCLOSURE>().IsHidden()) {
            SwapVisibility<MI_ENCLOSURE, MI_ENCLOSURE_ENABLE>();
        } else if (!xl_enclosure.isActive() && Item<MI_ENCLOSURE_ENABLE>().IsHidden()) {
            SwapVisibility<MI_ENCLOSURE_ENABLE, MI_ENCLOSURE>();
        }
#endif

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
