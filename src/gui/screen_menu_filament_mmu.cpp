/**
 * @file screen_menu_filament_mmu.cpp
 */

#include "screen_menu_filament_mmu.hpp"
#include "filament.hpp"
#include "filament_sensors_handler.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "img_resources.hpp"

ScreenMenuFilamentMMU::ScreenMenuFilamentMMU()
    : ScreenMenuFilamentMMU__(_(label)) {
    header.SetIcon(&img::spool_white_16x16);
}

void ScreenMenuFilamentMMU::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        bool filament_in_nozzle = FSensors_instance().WhereIsFilament() == MMU2::FilamentState::AT_FSENSOR;

        // Some operations are not available when filament is loaded all the
        // way to the nozzle (the MMU can't move then).
        //
        // Update the status ‒ both because it can change from a submenu and in
        // theory by a gcode sent remotely from eg. Connect.
        Item<MI_MMU_CUT_FILAMENT>().set_is_enabled(!filament_in_nozzle);
        Item<MI_MMU_EJECT_FILAMENT>().set_is_enabled(!filament_in_nozzle);
        Item<MI_MMU_PRELOAD>().set_is_enabled(!filament_in_nozzle);
        Item<MI_MMU_PRELOAD_ADVANCED>().set_is_enabled(!filament_in_nozzle);
    }

    ScreenMenu::windowEvent(sender, event, param);
}
