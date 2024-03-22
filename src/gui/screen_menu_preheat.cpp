/**
 * @file screen_menu_preheat.cpp
 */

#include "screen_menu_preheat.hpp"
#include "filament.hpp"
#include "filament_sensors_handler.hpp"

#include "img_resources.hpp"

#include "DialogHandler.hpp"

ScreenMenuPreheat::ScreenMenuPreheat()
    : ScreenMenuPreheat__(_(label)) {
#if (!PRINTER_IS_PRUSA_MINI)
    header.SetIcon(&img::preheat_white_16x16);
#endif // PRINTER_IS_PRUSA_MINI
}

void ScreenMenuPreheat::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            auto menu_index = menu.GetIndex();

            item->Do(); // do action (load filament ...)

            menu.SetIndex(menu_index.value_or(0)); // restore menu index
            header.SetText(_(label)); // restore label
        }
        return;
    }

    SuperWindowEvent(sender, event, param);
}
