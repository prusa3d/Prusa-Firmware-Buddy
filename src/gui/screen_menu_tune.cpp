/**
 * @file screen_menu_tune.cpp
 */

#include "screen_menu_tune.hpp"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "utility_extensions.hpp"
#include <option/has_mmu2.h>
#include <feature/cancel_object/cancel_object.hpp>

#if XL_ENCLOSURE_SUPPORT()
    #include "xl_enclosure.hpp"
#endif

ScreenMenuTune::ScreenMenuTune()
    : ScreenMenuTune__(_(label)) {
    ScreenMenuTune__::ClrMenuTimeoutClose();

#if HAS_MMU2()
    // Do not allow disabling filament sensor
    if (config_store().mmu2_enabled.get()) {
    #if HAS_FILAMENT_SENSORS_MENU()
        Item<MI_FILAMENT_SENSORS>().hide();
    #else
        Item<MI_FILAMENT_SENSOR>().hide();
    #endif
    }
#endif
}

void ScreenMenuTune::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::LOOP: {
#if XL_ENCLOSURE_SUPPORT()
        /* Once is Enclosure enabled in menu with ON/OFF switch (MI_ENCLOSURE_ENABLED), it tests the fan and if it passes, Enclosure is declared Active */
        /* If the test passes, MI_ENCLOSURE_ENABLE is swapped with MI_ENCLOSURE and enclosure settings can be accessed */
        /* This hides enclosure settings for Users without enclosure */

        if (xl_enclosure.is_active() && Item<MI_ENCLOSURE>().IsHidden()) {
            SwapVisibility<MI_ENCLOSURE, MI_ENCLOSURE_ENABLE>();
        } else if (!xl_enclosure.is_active() && Item<MI_ENCLOSURE_ENABLE>().IsHidden()) {
            SwapVisibility<MI_ENCLOSURE_ENABLE, MI_ENCLOSURE>();
        }
#endif

#if HAS_CANCEL_OBJECT()
        // Enable cancel object menu
        Item<MI_CO_CANCEL_OBJECT>().set_enabled(buddy::cancel_object().object_count() > 0);
#endif
        break;
    }

    default:
        break;
    }
    ScreenMenu::windowEvent(sender, event, param);
}
