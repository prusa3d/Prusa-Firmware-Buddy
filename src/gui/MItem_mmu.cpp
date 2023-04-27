#include "MItem_mmu.hpp"
#include "ScreenHandler.hpp"
#include "screen_messages.hpp"
#include "marlin_client.hpp"
#include "menu_spin_config.hpp"
#include "gui_fsensor_api.hpp"
#include "window_msgbox.hpp"
#include "eeprom.h"

#include "screen_menu_mmu_load_filament.hpp"
#include "screen_menu_mmu_eject_filament.hpp"
#include "screen_menu_mmu_cut_filament.hpp"
#include "screen_menu_mmu_load_to_nozzle.hpp"
#include "screen_menu_mmu_fail_stats.hpp"

/**********************************************************************************************/
// MI_MMU_LOAD_FILAMENT
MI_MMU_LOAD_FILAMENT::MI_MMU_LOAD_FILAMENT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_LOAD_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMULoadFilament>);
}

/**********************************************************************************************/
// MI_MMU_EJECT_FILAMENT
MI_MMU_EJECT_FILAMENT::MI_MMU_EJECT_FILAMENT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_EJECT_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMUEjectFilament>);
}

/**********************************************************************************************/
// MI_MMU_CUT_FILAMENT
MI_MMU_CUT_FILAMENT::MI_MMU_CUT_FILAMENT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_CUT_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMUCutFilament>);
}

/**********************************************************************************************/
// MI_MMU_LOAD_TO_NOZZLE
MI_MMU_LOAD_TO_NOZZLE::MI_MMU_LOAD_TO_NOZZLE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_LOAD_TO_NOZZLE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMULoadToNozzle>);
}

/**********************************************************************************************/
// MI_MMU_LOAD_FILAMENT_base
MI_MMU_ISSUE_GCODE::MI_MMU_ISSUE_GCODE(const char *lbl, const char *gcode)
    : WI_LABEL_t(_(lbl), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , gcode(gcode) {
}

void MI_MMU_ISSUE_GCODE::click(IWindowMenu & /*window_menu*/) {
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    marlin_gcode(gcode);
    //    while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
    //        marlin_client_loop();
    // gui_dlg_wait(gui_marlin_G28_or_G29_in_progress); // @@TODO perform some blocking wait on the LCD until the MMU finishes its job
    // Meanwhile an MMU error screen may occur!
}

/**********************************************************************************************/
// MI_MMU_LOAD_ALL
MI_MMU_LOAD_ALL::MI_MMU_LOAD_ALL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MMU_LOAD_ALL::click(IWindowMenu & /*window_menu*/) {
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    for (uint8_t i = 0; i < 5; ++i) {
        char gcode[] = "M704 Px";
        gcode[sizeof(gcode) - 2] = i + '0';
        marlin_gcode(gcode);
    }
}

/**********************************************************************************************/
// MI_MMU_ENABLE
MI_MMU_ENABLE::MI_MMU_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(FSensors_instance().HasMMU(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_MMU_ENABLE::OnChange(size_t old_index) {
    if (old_index) {
        FSensors_instance().DisableSideSensor();
    } else {
        switch (FSensors_instance().EnableSide()) {
        case filament_sensor::mmu_enable_result_t::ok:
            break;
        case filament_sensor::mmu_enable_result_t::error_filament_sensor_disabled:
            index = old_index;
            MsgBoxWarning(_("Can't enable MMU: enable the printer's filament sensor first."), Responses_Ok);
            break;
        case filament_sensor::mmu_enable_result_t::error_mmu_not_supported:
            index = old_index;
            MsgBoxError(_("MMU not supported!"), Responses_Ok);
            break;
        }
    }

    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)(EventMask::value | this->index));
}

/**********************************************************************************************/
// MI_MMU_SPOOLJOIN
MI_MMU_SPOOLJOIN::MI_MMU_SPOOLJOIN()
    : WI_ICON_SWITCH_OFF_ON_t(/*Screens::Access()->GetSpoolJoin() ? 1 : 0*/ 1, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_MMU_SPOOLJOIN::OnChange(size_t old_index) {
    if (!old_index) {
        //Screens::Access()->EnableSpoolJoin();
    } else {
        //Screens::Access()->DisableSpoolJoin();
    }
    // eeprom_set_bool(EEVAR_MENU_SPOOLJOIN, Screens::Access()->GetSpoolJoin());
}

/**********************************************************************************************/
// MI_MMU_CUTTER
MI_MMU_CUTTER::MI_MMU_CUTTER()
    : WI_ICON_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_MMU2_CUTTER), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_MMU_CUTTER::OnChange([[maybe_unused]] size_t old_index) {
    bool newState = !eeprom_get_bool(EEVAR_MMU2_CUTTER);
    // @@TODO some notification to the MMU2's engine?
    eeprom_set_bool(EEVAR_MMU2_CUTTER, newState);
}

/**********************************************************************************************/
// MI_MMU_STEALTH_MODE
MI_MMU_STEALTH_MODE::MI_MMU_STEALTH_MODE()
    : WI_ICON_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_MMU2_STEALTH_MODE), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_MMU_STEALTH_MODE::OnChange([[maybe_unused]] size_t old_index) {
    bool newState = !eeprom_get_bool(EEVAR_MMU2_STEALTH_MODE);
    // @@TODO some notification to the MMU2's engine?
    eeprom_set_bool(EEVAR_MMU2_STEALTH_MODE, newState);
}

/**********************************************************************************************/
// MI_MMU_FAIL_STATS
MI_MMU_FAIL_STATS::MI_MMU_FAIL_STATS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_FAIL_STATS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMUFailStats>);
}
