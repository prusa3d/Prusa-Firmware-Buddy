/**
 * @file screen_menu_filament_mmu.cpp
 */

#include "screen_menu_filament_mmu.hpp"
#include "filament.hpp"
#include "filament_sensors_handler.hpp"
#include "marlin_client.hpp"
#include "window_dlg_load_unload.hpp"
#include "ScreenHandler.hpp"
#include "DialogHandler.hpp"
#include "sound.hpp"
#include "png_resources.hpp"

ScreenMenuFilamentMMU::ScreenMenuFilamentMMU()
    : ScreenMenuFilamentMMU__(_(label)) {
    header.SetIcon(&png::spool_white_16x16);
    ClrMenuTimeoutClose(); // don't close on menu timeout
    deactivate_item();
}

void ScreenMenuFilamentMMU::deactivate_item() {
    auto current_filament = filament::get_type_in_extruder(marlin_vars()->active_extruder);
    current_filament == filament::Type::NONE ? DisableItem<MI_PURGE>() : EnableItem<MI_PURGE>();
}

void ScreenMenuFilamentMMU::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    deactivate_item();
    if (event == GUI_event_t::CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            item->Do();               //do action (load filament ...)
            header.SetText(_(label)); //restore label
        }
        return;
    }
}
