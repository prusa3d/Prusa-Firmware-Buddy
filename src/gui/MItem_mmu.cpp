#include "MItem_mmu.hpp"
#include "ScreenHandler.hpp"
#include "screen_messages.hpp"
#include "marlin_client.hpp"
#include "WindowMenuSpin.hpp"
#include "window_msgbox.hpp"
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <mmu2/fail_bucket.hpp>

#if HAS_SELFTEST()
    #include "ScreenSelftest.hpp"
#endif

#include "screen_menu_mmu_load_test_filament.hpp"
#include "screen_menu_mmu_eject_filament.hpp"
#include "screen_menu_mmu_cut_filament.hpp"
#include "screen_menu_mmu_load_to_nozzle.hpp"
#include "screen/screen_menu_mmu_preload_all.hpp"
#include "screen_menu_filament_changeall.hpp"

#include <config_store/store_instance.hpp>
#include <feature/prusa/MMU2/mmu2_mk4.h>
#include <gui/screen_printer_setup.hpp>
#include <MItem_tools.hpp>

/**********************************************************************************************/
// MI_MMU_PRELOAD
MI_MMU_PRELOAD::MI_MMU_PRELOAD()
    : IWindowMenuItem(_(label), nullptr,
        // enable the PreLoad menu only if there is no filament already loaded
        FSensors_instance().WhereIsFilament() == MMU2::FilamentState::AT_FSENSOR ? is_enabled_t::no : is_enabled_t::yes,
        is_hidden_t::no,
        expands_t::yes) {
}
void MI_MMU_PRELOAD::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenChangeAllFilaments>);
}

/**********************************************************************************************/
// MI_MMU_LOAD_TEST_FILAMENT
MI_MMU_LOAD_TEST_FILAMENT::MI_MMU_LOAD_TEST_FILAMENT()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, MMU2::mmu2.Enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}
void MI_MMU_LOAD_TEST_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMULoadTestFilament>);
}

/**********************************************************************************************/
// MI_MMU_EJECT_FILAMENT
MI_MMU_EJECT_FILAMENT::MI_MMU_EJECT_FILAMENT()
    : IWindowMenuItem(_(label), nullptr,
        // enable the eject menu only if there is no filament already loaded
        FSensors_instance().WhereIsFilament() == MMU2::FilamentState::AT_FSENSOR ? is_enabled_t::no : is_enabled_t::yes,
        is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_EJECT_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMUEjectFilament>);
}

/**********************************************************************************************/
// MI_MMU_CUT_FILAMENT
MI_MMU_CUT_FILAMENT::MI_MMU_CUT_FILAMENT()
    : IWindowMenuItem(_(label), nullptr,
        FSensors_instance().WhereIsFilament() == MMU2::FilamentState::AT_FSENSOR ? is_enabled_t::no : is_enabled_t::yes,
        is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_CUT_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMUCutFilament>);
}

/**********************************************************************************************/
// MI_MMU_LOAD_TO_NOZZLE
MI_MMU_LOAD_TO_NOZZLE::MI_MMU_LOAD_TO_NOZZLE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}
void MI_MMU_LOAD_TO_NOZZLE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMULoadToNozzle>);
}

/**********************************************************************************************/
// MI_MMU_ISSUE_GCODE
MI_MMU_ISSUE_GCODE::MI_MMU_ISSUE_GCODE(const char *lbl, const char *gcode, is_hidden_t hidden)
    : IWindowMenuItem(_(lbl), nullptr, is_enabled_t::yes, hidden)
    , gcode(gcode) {
}

void MI_MMU_ISSUE_GCODE::click(IWindowMenu & /*window_menu*/) {
    marlin_client::gcode(gcode);
    // gui_dlg_wait(gui_marlin_G28_or_G29_in_progress); // @@TODO perform some blocking wait on the LCD until the MMU finishes its job
    // Meanwhile an MMU error screen may occur!
}

/**********************************************************************************************/
// MI_MMU_ITEM_WITH_SLOT
MI_MMU_ITEM_WITH_SLOT::MI_MMU_ITEM_WITH_SLOT(uint8_t slot_i, const char *label_prefix)
    : slot_i_(slot_i) {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    std::array<char, 32> translated_prefix;
    _(label_prefix).copyToRAM(translated_prefix.data(), translated_prefix.size());
    snprintf(label_.data(), label_.size(), "%s %i", translated_prefix.data(), slot_i + 1);
    SetLabel(string_view_utf8::MakeRAM(label_.data()));
#pragma GCC diagnostic pop
}

/**********************************************************************************************/
// MI_MMU_ISSUE_GCODE_SLOT_FMT
MI_MMU_ISSUE_GCODE_SLOT_FMT::MI_MMU_ISSUE_GCODE_SLOT_FMT(uint8_t slot_i, const char *label_prefix, const char *gcode_fmt)
    : MI_MMU_ITEM_WITH_SLOT(slot_i, label_prefix)
    , gcode_fmt_(gcode_fmt) {
}

void MI_MMU_ISSUE_GCODE_SLOT_FMT::click(IWindowMenu &) {
    std::array<char, MAX_CMD_SIZE> gcode;
    snprintf(gcode.data(), gcode.size(), gcode_fmt_, slot_i_);
    gui_try_gcode_with_msg(gcode.data());
}

/**********************************************************************************************/
// MI_MMU_ISSUE_LOAD_TO_NOZZLE_SLOT
MI_MMU_ISSUE_LOAD_TO_NOZZLE_SLOT::MI_MMU_ISSUE_LOAD_TO_NOZZLE_SLOT(uint8_t slot_i, const char *label_prefix)
    : MI_MMU_ITEM_WITH_SLOT(slot_i, label_prefix) {}

void MI_MMU_ISSUE_LOAD_TO_NOZZLE_SLOT::click(IWindowMenu &) {
    std::array<char, MAX_CMD_SIZE> gcode;
    FilamentType f = config_store().get_filament_type(slot_i_);

    if (f == FilamentType::none) {
        // if, for some reason, no filament type is known on a specific slot, we shall omit the S parameter from the M701 gcode
        // Also, un-preloaded filament slots shall be grayed-out to prevent the user from getting into these edge-cases
        snprintf(gcode.data(), gcode.size(), "M701 W2 P%i", slot_i_);
    } else {
        snprintf(gcode.data(), gcode.size(), "M701 S\"%s\" W2 P%i", f.parameters().name.data(), slot_i_);
    }

    gui_try_gcode_with_msg(gcode.data());
}

/**********************************************************************************************/
// MI_MMU_PRELOAD_ALL
MI_MMU_PRELOAD_ALL::MI_MMU_PRELOAD_ALL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_MMU_PRELOAD_ALL::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMMUPreloadAll>);
}

/**********************************************************************************************/
// MI_MMU_LOAD_TEST_ALL
MI_MMU_LOAD_TEST_ALL::MI_MMU_LOAD_TEST_ALL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MMU_LOAD_TEST_ALL::click(IWindowMenu & /*window_menu*/) {
    for (uint8_t i = 0; i < 5; ++i) {
        char gcode[] = "M1704 Px M0";
        gcode[sizeof(gcode) - 2] = i + '0';
        marlin_client::gcode(gcode);
    }
}

/**
 * @brief Flips the value of the MMU Rework toggle.
 *
 * Displays a dialog warning the user about the FS behavior changing with this
 * switch. Then it sets the value of is_mmu_rework and runs FS calibration.
 */
static bool set_mmu_rework(bool set) {
    if (config_store().is_mmu_rework.get() == set) {
        return true;
    }

    if (MsgBoxWarning(_("This will change the behavior of the filament sensor. Do you want to continue?"), { Response::Continue, Response::Abort, Response::_none, Response::_none }) != Response::Continue) {
        return false;
    }

    // When enabling MMU rework, force set footer items
    // BFW-5219
    if (set) {
        StatusFooter::SetSlotInit(3, footer::Item::f_sensor);
        StatusFooter::SetSlotInit(4, footer::Item::finda);
    }

    config_store().is_mmu_rework.set(set);
    FSensors_instance().request_enable_state_update();

#if HAS_SELFTEST() && !PRINTER_IS_PRUSA_MK3_5()
    // Invalidate the extruder fsensor calibration
    FilamentSensorCalibrator::Storage storage;
    GetExtruderFSensor(0)->create_calibrator(storage)->finish();

    // And start the calibration right away
    marlin_client::gcode("M1981");
#endif

    return true;
}

/**********************************************************************************************/
// MI_MMU_ENABLE
MI_MMU_ENABLE::MI_MMU_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().mmu2_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MMU_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    if (value() && !config_store().is_mmu_rework.get() && !set_mmu_rework(true)) {
        set_value(false);
        return;

    } else if (value() && !FSensors_instance().is_working(LogicalFilamentSensor::extruder)) {
        MsgBoxWarning(_("Can't enable MMU: calibrate and enable the printer's filament sensor first."), Responses_Ok);
        set_value(false);
        return;
    }

    marlin_client::gcode(value() ? "M709 S1" : "M709 S0");
}

void MI_MMU_ENABLE::Loop() {
    set_value(config_store().mmu2_enabled.get());
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

static constexpr std::array mmu_rework_items = std::to_array<const char *>({
    N_("Stock"),
    "MMU",
});
/**********************************************************************************************/

// MMU INVOKE MAINTENANCE
MI_MMU_INVOKE_MAINTENANCE::MI_MMU_INVOKE_MAINTENANCE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_MMU_INVOKE_MAINTENANCE::click(IWindowMenu & /*window_menu*/) {
    config_store().mmu_fail_bucket.set(MMU2::FailLeakyBucket::default_overflow_limit);
}

MI_MMU_NEXTRUDER_REWORK::MI_MMU_NEXTRUDER_REWORK()
    : MenuItemSelectMenu(_(HAS_LOADCELL() ? "Nextruder" : N_("Extruder"))) {
    set_current_item(config_store().is_mmu_rework.get());
}

int MI_MMU_NEXTRUDER_REWORK::item_count() const {
    return mmu_rework_items.size();
}

string_view_utf8 MI_MMU_NEXTRUDER_REWORK::build_item_text(int index, [[maybe_unused]] MenuItemSelectMenu::ItemTextParams &params) const {
    return _(mmu_rework_items[index]);
}

bool MI_MMU_NEXTRUDER_REWORK::on_item_selected(const OnItemSelectedArgs &args) {
    if (!set_mmu_rework(args.new_index == 1)) {
        return false;
    }

    // Enabling MMU rework hides the FS_Autoload option from the menu - BFW-4290
    // However the request was that the autoload "stays active"
    // So we have to make sure that it's on when you activate the rework
    if (args.new_index) {
        config_store().fs_autoload_enabled.set(true);
    }

    return true;
};

/*****************************************************************************/
// MI_INFO_FINDA
MI_INFO_FINDA::MI_INFO_FINDA()
    : MenuItemAutoUpdatingLabel(
        string_view_utf8::MakeCPUFLASH("FINDA"),
        [this](const std::span<char> &buffer) {
            // TODO: change of visualization scheme is expected soon, some unification with fsensor visualization will happen as a result.
            // For now, FINDA is visualized the same way like filament sensors' states
            _(value() ? N_(" INS / 1") : N_("NINS / 0")).copyToRAM(buffer);
        },
        [](auto) -> bool {
            return marlin_vars().mmu2_finda.get();
        } //
    ) {
    set_is_hidden(!MMU2::mmu2.Enabled());
}

/*****************************************************************************/
// MI_MMU_BOOTLOADER_RESULT

static void MI_MMU_BOOTLOADER_RESULT_label(const std::span<char> &buffer) {
    string_view_utf8 result = [&] {
        if (!MMU2::mmu2.Enabled()) {
            return _("MMU off");
        }

        static constexpr EnumArray<MMU2BootloaderResult, const char *, 5> mmu2_bootloader_result_texts {
            { MMU2BootloaderResult::not_detected, N_("Not detected") },
            { MMU2BootloaderResult::fw_up_to_date, N_("FW ok") },
            { MMU2BootloaderResult::fw_updated, N_("FW updated") },
            { MMU2BootloaderResult::flashing_error, N_("Flashing err") },
            { MMU2BootloaderResult::comm_error, N_("Comm err") },
        };
        return _(mmu2_bootloader_result_texts[MMU2::mmu2.bootloader_result()]);
    }();

    result.copyToRAM(buffer);
}

MI_MMU_BOOTLOADER_RESULT::MI_MMU_BOOTLOADER_RESULT()
    : WI_FORMATABLE_LABEL_t<bool>(
        _(label), nullptr, is_enabled_t::yes, config_store().mmu2_enabled.get() ? is_hidden_t::dev : is_hidden_t::yes, false, &MI_MMU_BOOTLOADER_RESULT_label) {}

MI_DONE_EXTRUDER_MAINTENANCE::MI_DONE_EXTRUDER_MAINTENANCE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, config_store().is_mmu_rework.get() ? is_hidden_t::no : is_hidden_t::yes, expands_t::no) {
}

void MI_DONE_EXTRUDER_MAINTENANCE::click(IWindowMenu &) {
#if HAS_LOADCELL()
    static constexpr char const *msg = N_("Do you want to reset the Nextruder main-plate maintenance reminder?");
#else
    static constexpr char const *msg = N_("Do you want to reset the extruder maintenance reminder?");
#endif
    if (MsgBoxQuestion(_(msg), Responses_YesNo) == Response::Yes) {
        config_store().mmu_last_maintenance.set(config_store().mmu_changes.get());
        config_store().mmu_fail_bucket.set(0);
    }
}
