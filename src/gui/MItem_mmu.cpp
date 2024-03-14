#include "MItem_mmu.hpp"
#include "ScreenHandler.hpp"
#include "screen_messages.hpp"
#include "marlin_client.hpp"
#include "menu_spin_config.hpp"
#include "gui_fsensor_api.hpp"
#include "window_msgbox.hpp"

#include "screen_menu_mmu_preload_to_mmu.hpp"
#include "screen_menu_mmu_load_test_filament.hpp"
#include "screen_menu_mmu_eject_filament.hpp"
#include "screen_menu_mmu_cut_filament.hpp"
#include "screen_menu_mmu_load_to_nozzle.hpp"

#include <config_store/store_instance.hpp>
#include <feature/prusa/MMU2/mmu2_mk4.h>

/**********************************************************************************************/
// MI_MMU_LOAD_FILAMENT
MI_MMU_PRELOAD::MI_MMU_PRELOAD()
    : WI_LABEL_t(_(label), nullptr,
        // enable the PreLoad menu only if there is no filament already loaded
        FSensors_instance().WhereIsFilament() == MMU2::FilamentState::AT_FSENSOR ? is_enabled_t::no : is_enabled_t::yes,
        is_hidden_t::no,
        expands_t::yes) {
}
void MI_MMU_PRELOAD::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMUPreloadToMMU>);
}

/**********************************************************************************************/
// MI_MMU_LOAD_TEST_FILAMENT
MI_MMU_LOAD_TEST_FILAMENT::MI_MMU_LOAD_TEST_FILAMENT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_LOAD_TEST_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMULoadTestFilament>);
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
    marlin_client::event_clr(marlin_server::Event::CommandBegin);
    marlin_client::gcode(gcode);
    //    while (!marlin_client::event_clr(Event::CommandBegin))
    //        marlin_client::loop();
    // gui_dlg_wait(gui_marlin_G28_or_G29_in_progress); // @@TODO perform some blocking wait on the LCD until the MMU finishes its job
    // Meanwhile an MMU error screen may occur!
}

/**********************************************************************************************/
// MI_MMU_PRELOAD_ALL
MI_MMU_PRELOAD_ALL::MI_MMU_PRELOAD_ALL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MMU_PRELOAD_ALL::click(IWindowMenu & /*window_menu*/) {
    marlin_client::event_clr(marlin_server::Event::CommandBegin);
    for (uint8_t i = 0; i < 5; ++i) {
        char gcode[] = "M704 Px";
        gcode[sizeof(gcode) - 2] = i + '0';
        marlin_client::gcode(gcode);
    }
}

/**********************************************************************************************/
// MI_MMU_LOAD_TEST_ALL
MI_MMU_LOAD_TEST_ALL::MI_MMU_LOAD_TEST_ALL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MMU_LOAD_TEST_ALL::click(IWindowMenu & /*window_menu*/) {
    marlin_client::event_clr(marlin_server::Event::CommandBegin);
    for (uint8_t i = 0; i < 5; ++i) {
        char gcode[] = "M1704 Px";
        gcode[sizeof(gcode) - 2] = i + '0';
        marlin_client::gcode(gcode);
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
// MI_MMU_CUTTER
MI_MMU_CUTTER::MI_MMU_CUTTER()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().mmu2_cutter.get(), _(label), nullptr, is_enabled_t::yes, MMU2::mmu2.Enabled() ? is_hidden_t::no : is_hidden_t::yes) {}
void MI_MMU_CUTTER::OnChange([[maybe_unused]] size_t old_index) {
    bool newState = !config_store().mmu2_cutter.get();
    // @@TODO some notification to the MMU2's engine?
    config_store().mmu2_cutter.set(newState);
}

/**********************************************************************************************/
// MI_MMU_STEALTH_MODE
MI_MMU_STEALTH_MODE::MI_MMU_STEALTH_MODE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().mmu2_stealth_mode.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_MMU_STEALTH_MODE::OnChange([[maybe_unused]] size_t old_index) {
    bool newState = !config_store().mmu2_stealth_mode.get();
    // @@TODO some notification to the MMU2's engine?
    config_store().mmu2_stealth_mode.set(newState);
}

/**********************************************************************************************/
// MMU FAIL STATS
MI_MMU_LOAD_FAILS::MI_MMU_LOAD_FAILS()
    : WI_INFO_t(config_store().mmu2_load_fails.get(), _(label), MMU2::mmu2.Enabled() ? is_hidden_t::no : is_hidden_t::yes) {}

MI_MMU_TOTAL_LOAD_FAILS::MI_MMU_TOTAL_LOAD_FAILS()
    : WI_INFO_t(config_store().mmu2_total_load_fails.get(), _(label), MMU2::mmu2.Enabled() ? is_hidden_t::no : is_hidden_t::yes) {}

MI_MMU_GENERAL_FAILS::MI_MMU_GENERAL_FAILS()
    : WI_INFO_t(config_store().mmu2_fails.get(), _(label), MMU2::mmu2.Enabled() ? is_hidden_t::no : is_hidden_t::yes) {}

MI_MMU_TOTAL_GENERAL_FAILS::MI_MMU_TOTAL_GENERAL_FAILS()
    : WI_INFO_t(config_store().mmu2_total_fails.get(), _(label), MMU2::mmu2.Enabled() ? is_hidden_t::no : is_hidden_t::yes) {}
