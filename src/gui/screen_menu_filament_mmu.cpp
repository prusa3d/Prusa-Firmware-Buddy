/**
 * @file screen_menu_filament_mmu.cpp
 */

#include "screen_menu_filament_mmu.hpp"
#include "filament.hpp"
#include "filament_sensors_handler.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include "DialogHandler.hpp"
#include "sound.hpp"
#include "img_resources.hpp"

ScreenMenuFilamentMMU::ScreenMenuFilamentMMU()
    : ScreenMenuFilamentMMU__(_(label)) {
    header.SetIcon(&img::spool_white_16x16);
    ClrMenuTimeoutClose(); // don't close on menu timeout
}

void ScreenMenuFilamentMMU::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            item->Do(); // do action (load filament ...)
            header.SetText(_(label)); // restore label
        }
        return;
    }
}
