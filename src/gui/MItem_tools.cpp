#include "MItem_tools.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "gui.hpp"
#include "time_helper.hpp"
#include "sys.h"
#include "window_dlg_wait.hpp"
#include "window_dlg_calib_z.hpp"
#include "window_file_list.hpp"
#include "sound.hpp"
#include "wui_api.h"
#include "printers.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "filament_sensors_handler.hpp"
#include "liveadjust_z.hpp"
#include "filament_sensor.hpp"
#include "main.h"
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "config.h"
#include "WindowMenuSpin.hpp"
#include "time_tools.hpp"
#include "footer_eeprom.hpp"
#include "version.h"
#include "../../common/PersistentStorage.h"
#include "sys.h"
#include "w25x.h"
#include <bootloader/bootloader.hpp>
#include "config_features.h"
#include <config_store/store_instance.hpp>
#include "connect/marlin_printer.hpp"
#include <crash_dump/dump.hpp>
#include <feature/prusa/e-stall_detector.h>
#include <option/bootloader.h>
#include <option/filament_sensor.h>
#include <option/has_phase_stepping.h>
#include <option/has_side_fsensor.h>
#include <option/has_side_leds.h>
#include <option/has_coldpull.h>
#include <RAII.hpp>
#include <st25dv64k.h>
#include <time.h>
#include <footer_items_heaters.hpp>
#include <footer_line.hpp>
#include <freertos/critical_section.hpp>
#include <str_utils.hpp>
#include <netdev.h>
#include <wui.h>
#include <power_panic.hpp>

#include <type_traits>

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "../../../lib/Marlin/Marlin/src/module/prusa/toolchanger.h"
    #include "screen_menu_tools.hpp"
    #include <window_tool_action_box.hpp>
#endif

#if HAS_LEDS()
    #include <led_animations/animator.hpp>
#endif

#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif

#if BUDDY_ENABLE_CONNECT()
    #include <connect/marlin_printer.hpp>
#endif

namespace {
void MsgBoxNonBlockInfo(const string_view_utf8 &txt) {
    constexpr static const char *title = N_("Information");
    MsgBoxTitled mbt(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, txt, is_multiline::yes, _(title), &img::info_16x16);
    gui::TickLoop();
    gui_loop();
}

constexpr const char *homing_text_info = N_("Printer may vibrate and be noisier during homing.");
constexpr const char *printer_busy_text = N_("Printer is busy. Please try repeating the action later.");

} // namespace

bool gui_check_space_in_gcode_queue_with_msg() {
    if (marlin_vars().gqueue <= MEDIA_FETCH_GCODE_QUEUE_FILL_TARGET) {
        return true;
    }

    MsgBoxWarning(_(printer_busy_text), Responses_Ok);
    return false;
}

bool gui_try_gcode_with_msg(const char *gcode) {
    switch (marlin_client::gcode_try(gcode)) {

    case marlin_client::GcodeTryResult::Submitted:
        return true;

    case marlin_client::GcodeTryResult::QueueFull:
        MsgBoxWarning(_(printer_busy_text), Responses_Ok);
        return false;

    case marlin_client::GcodeTryResult::GcodeTooLong:
        bsod("Gcode too long");
    }

    return false;
}

/**********************************************************************************************/
// MI_FILAMENT_SENSOR
MI_FILAMENT_SENSOR::MI_FILAMENT_SENSOR()
    : WI_ICON_SWITCH_OFF_ON_t(0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
    update();
}

void MI_FILAMENT_SENSOR::update() {
    SetIndex(config_store().fsensor_enabled.get());
}

void MI_FILAMENT_SENSOR::OnChange(size_t old_index) {
    // Enabling/disabling FS can generate gcodes (I'm looking at you, MMU!).
    // Fail the action if there's no space in the queue.
    if (!gui_check_space_in_gcode_queue_with_msg()) {
        // SetIndex doesn't call OnChange
        SetIndex(old_index);
        return;
    }

    auto &fss = FSensors_instance();
    fss.set_enabled_global(index);

    if (index && !fss.gui_wait_for_init_with_msg()) {
        FSensors_instance().set_enabled_global(false);
        SetIndex(old_index);
    }

    // Signal to the parent to check for changed
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, nullptr);
}

/*****************************************************************************/
// MI_STUCK_FILAMENT_DETECTION
/*****************************************************************************/
bool MI_STUCK_FILAMENT_DETECTION::init_index() const {
    return config_store().stuck_filament_detection.get();
}

void MI_STUCK_FILAMENT_DETECTION::OnChange(size_t old_index) {
    if (!gui_try_gcode_with_msg(value() ? "M591 S1 P" : "M591 S0 P")) {
        set_value(old_index, false);
    }
}

/*****************************************************************************/
// MI_STEALTH_MODE
/*****************************************************************************/
MI_STEALTH_MODE::MI_STEALTH_MODE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().stealth_mode.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_STEALTH_MODE::OnChange(size_t old_index) {
    if (!gui_try_gcode_with_msg(value() ? "M9150" : "M9140")) {
        set_value(old_index, false);
    }
}

/*****************************************************************************/
// MI_LIVE_ADJUST_Z
MI_LIVE_ADJUST_Z::MI_LIVE_ADJUST_Z()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes,
#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_MK3_5()
        is_hidden_t::no
#else
        is_hidden_t::dev
#endif
    ) {
}

void MI_LIVE_ADJUST_Z::click(IWindowMenu & /*window_menu*/) {
    LiveAdjustZ::Show();
}

/*****************************************************************************/
// MI_AUTO_HOME
MI_AUTO_HOME::MI_AUTO_HOME()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_AUTO_HOME::click(IWindowMenu & /*window_menu*/) {
    marlin_client::event_clr(marlin_server::Event::CommandBegin);
    marlin_client::gcode("G28");
    while (!marlin_client::event_clr(marlin_server::Event::CommandBegin)) {
        marlin_client::loop();
    }
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress, _(homing_text_info));
}

/*****************************************************************************/
// MI_MESH_BED
MI_MESH_BED::MI_MESH_BED()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MESH_BED::click(IWindowMenu & /*window_menu*/) {
    if (!marlin_server::all_axes_homed()) {
        marlin_client::event_clr(marlin_server::Event::CommandBegin);
        marlin_client::gcode("G28");
        while (!marlin_client::event_clr(marlin_server::Event::CommandBegin)) {
            marlin_client::loop();
        }
        gui_dlg_wait(gui_marlin_G28_or_G29_in_progress, _(homing_text_info));
    }
    marlin_client::event_clr(marlin_server::Event::CommandBegin);
    marlin_client::gcode("G29");
    while (!marlin_client::event_clr(marlin_server::Event::CommandBegin)) {
        marlin_client::loop();
    }
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
}

/*****************************************************************************/
// MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_DISABLE_STEP::click(IWindowMenu & /*window_menu*/) {
#if (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_MK3_5())
    marlin_client::gcode("M18 X Y E");
#else
    marlin_client::gcode("M18");
#endif
}

/*****************************************************************************/

namespace {
void st25dv64k_chip_erase() {
    static constexpr uint32_t empty = 0xffffffff;
    for (uint16_t address = 0; address <= (8096 - 4); address += 4) {
        st25dv64k_user_write_bytes(address, &empty, sizeof(empty));
    }
}

void msg_and_sys_reset() {
    MsgBoxInfo(_("Reset complete. The system will now restart."), Responses_Ok);
    sys_reset();
}

void do_factory_reset(bool wipe_fw) {
    auto msg = MsgBoxBase(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, wipe_fw ? _("Erasing everything,\nit will take some time...") : _("Erasing configuration,\nit will take some time..."));
    msg.Draw(); // Non-blocking info
    st25dv64k_chip_erase();
    if (wipe_fw) {
        w25x_chip_erase();
#if BOOTLOADER()
        // Invalidate firmware by erasing part of it
        if (!buddy::bootloader::fw_invalidate()) {
            bsod("Error invalidating firmware");
        }
        // Never gets here
#endif /*BOOTLOADER()*/
    }
    msg_and_sys_reset();
}

#if PRINTER_IS_PRUSA_MK4()
void do_shipping_prep() {
    auto msg = MsgBoxBase(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, // a dummy comment to break line by force
        _("Shipping preparation\n\nErasing configuration\n(but keeping Nextruder type)\nit will take some time..."));
    msg.Draw(); // Non-blocking info

    const auto is_mmu_rework = config_store().is_mmu_rework.get();
    const auto nozzle_is_high_flow = config_store().nozzle_is_high_flow.get();
    const auto ext_printer_type = config_store().extended_printer_type.get();

    // at this spot, we hope, that no other thread is actually writing into the EEPROM. They can read, even though it doesn't probably happen
    // we cannot disable task switching here because osDelays stop working ... which is required for erasing the EEPROM
    st25dv64k_chip_erase();
    {
        // disable task switching now while reloading the EEPROM structures
        freertos::CriticalSection critical_section;

        // Build the structures again - this is the tricky part.
        // What an awful way of force-reinitializing the RAM data structures of config_store
        // - unfortunately it gobbled up 3KB of code space. It would be nice to find a more subtle impl.
        using Store = std::remove_cvref_t<decltype(config_store())>;
        config_store().~Store();
        new (&config_store()) Store();

        init_config_store();
        config_store().perform_config_check();
    }

    // write back the flags we want to keep
    config_store().is_mmu_rework.set(is_mmu_rework);
    config_store().extended_printer_type.set(ext_printer_type);
    config_store().nozzle_is_high_flow.set(nozzle_is_high_flow);

    msg_and_sys_reset();
}
#endif

} // anonymous namespace

MI_FACTORY_SOFT_RESET::MI_FACTORY_SOFT_RESET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_SOFT_RESET::click(IWindowMenu & /*window_menu*/) {
    if (MsgBoxWarning(_("This operation cannot be undone. Current user configuration and passwords will be lost!\nDo you want to reset the printer to factory defaults?"), Responses_YesNo, 1) == Response::Yes) {
        do_factory_reset(false);
    }
}

MI_FACTORY_HARD_RESET::MI_FACTORY_HARD_RESET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_HARD_RESET::click(IWindowMenu & /*window_menu*/) {
    static constexpr const char *fmt2Translate = N_("This operation cannot be undone. Current configuration will be lost!\nYou will need a USB drive with this firmware (%s_firmware_%s.bbf file) to start the printer again.\nDo you really want to continue?");

    StringViewUtf8Parameters<20> params;
    const string_view_utf8 str = _(fmt2Translate).formatted(params, PRINTER_MODEL, project_version);

    if (MsgBoxWarning(str, Responses_YesNo, 1) == Response::Yes) {
        do_factory_reset(true);
    }
}

#if PRINTER_IS_PRUSA_MK4()
MI_FACTORY_SHIPPING_PREP::MI_FACTORY_SHIPPING_PREP()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_SHIPPING_PREP::click(IWindowMenu & /*window_menu*/) {
    if (MsgBoxWarning(_("This operation cannot be undone. Current configuration will be lost!\nDo you want to perform the Factory Shipping Preparation procedure?"), Responses_YesNo, 1) == Response::Yes) {
        do_shipping_prep();
    }
}
#endif

/*****************************************************************************/
// MI_ENTER_DFU
#ifdef BUDDY_ENABLE_DFU_ENTRY
MI_ENTER_DFU::MI_ENTER_DFU()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_ENTER_DFU::click(IWindowMenu &) {
    sys_dfu_request_and_reset();
}
#endif

/*****************************************************************************/
// MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SAVE_DUMP::click(IWindowMenu & /*window_menu*/) {
    MsgBoxNonBlockInfo(_("A crash dump is being saved."));
    if (!crash_dump::dump_is_valid()) {
        MsgBoxInfo(_("No crash dump to save."), Responses_Ok);
    } else if (crash_dump::save_dump_to_usb("/usb/dump.bin")) {
        MsgBoxInfo(_("A crash dump report (file dump.bin) has been saved to the USB drive."), Responses_Ok);
    } else {
        MsgBoxError(_("Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again."), Responses_Ok);
    }
}

/*****************************************************************************/
// MI_XFLASH_RESET
MI_XFLASH_RESET::MI_XFLASH_RESET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_XFLASH_RESET::click(IWindowMenu & /*window_menu*/) {
    crash_dump::dump_reset();
}

/*****************************************************************************/
// MI_EE_SAVEXML
MI_EE_SAVEXML::MI_EE_SAVEXML()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_SAVEXML::click(IWindowMenu & /*window_menu*/) {
    // eeprom_save_xml_to_usb("/usb/eeprom.xml"); // TODO(ConfigStore): Not yet migrated
}

/*****************************************************************************/
// MI_EE_CLEAR
MI_EE_CLEAR::MI_EE_CLEAR()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_EE_CLEAR::click(IWindowMenu & /*window_menu*/) {
    static constexpr uint32_t empty = 0xffffffff;
    for (uint16_t address = 0; address <= (8096 - 4); address += 4) {
        st25dv64k_user_write_bytes(address, &empty, sizeof(empty));
    }
}

/*****************************************************************************/
// MI_M600
MI_M600::MI_M600()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_M600::click(IWindowMenu & /*window_menu*/) {
    if (!enqueued) {
        marlin_client::inject("M600");
        enqueued = true;
    }
}

/*****************************************************************************/
// MI_DRYRUN
MI_DRYRUN::MI_DRYRUN()
    : WI_ICON_SWITCH_OFF_ON_t((marlin_debug_flags & MARLIN_DEBUG_DRYRUN) ? 1 : 0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_DRYRUN::OnChange(size_t) {
    // marlin_debug_flags should be accessed only from the marlin thread.
    // Ideally the M111 should be expanded for setting/resetting individual bits, but:
    // * this menu item is dev-only
    // * there's not much this can screw up
    // * this is actually safer, because the read and write is close together (when issuing M111 with all flags override, there's more change of a race condition)

    if (value()) {
        marlin_debug_flags |= MARLIN_DEBUG_DRYRUN;
    } else {
        marlin_debug_flags &= ~MARLIN_DEBUG_DRYRUN;
    }
}

/*****************************************************************************/
// MI_TIMEOUT
MI_TIMEOUT::MI_TIMEOUT()
    : WI_ICON_SWITCH_OFF_ON_t(Screens::Access()->GetMenuTimeout() ? 1 : 0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEOUT::OnChange(size_t old_index) {
    if (!old_index) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }
    config_store().menu_timeout.set(static_cast<uint8_t>(Screens::Access()->GetMenuTimeout()));
}

/*****************************************************************************/
// MI_SOUND_MODE
size_t MI_SOUND_MODE::init_index() const {
    eSOUND_MODE sound_mode = Sound_GetMode();
    return (size_t)(sound_mode > eSOUND_MODE::_last ? eSOUND_MODE::_default_sound : sound_mode);
}
MI_SOUND_MODE::MI_SOUND_MODE()
    : WI_SWITCH_t<MI_SOUND_MODE_COUNT>(init_index(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no,
        _(str_Once), _(str_Loud), _(str_Silent), _(str_Assist)
#ifdef _DEBUG
                                                     ,
        string_view_utf8::MakeCPUFLASH((const uint8_t *)str_Debug)
#endif
    ) {
}

void MI_SOUND_MODE::OnChange(size_t /*old_index*/) {
    Sound_SetMode(static_cast<eSOUND_MODE>(index));
}

/*****************************************************************************/
// MI_SOUND_TYPE
MI_SOUND_TYPE::MI_SOUND_TYPE()
    : WI_SWITCH_t<8>(0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no,
        _(str_ButtonEcho), _(str_StandardPrompt), _(str_StandardAlert), _(str_CriticalAlert),
        _(str_EncoderMove), _(str_BlindAlert), _(str_Start), _(str_SingleBeep)) {}
void MI_SOUND_TYPE::OnChange(size_t old_index) {
    eSOUND_TYPE st = static_cast<eSOUND_TYPE>(old_index);
    if (st == eSOUND_TYPE::StandardPrompt || st == eSOUND_TYPE::CriticalAlert) {
        Sound_Play(eSOUND_TYPE::StandardPrompt);
        // this is a debug-only menu item, intentionally not translated
        static const uint8_t msg[] = "Continual beeps test\n press button to stop";
        MsgBoxInfo(string_view_utf8::MakeCPUFLASH(msg), Responses_Ok);
        Sound_Stop();
    } else {
        Sound_Play(st);
    }
}

/*****************************************************************************/
// MI_SOUND_VOLUME
static constexpr NumericInputConfig sound_volume_spin_config = {
    .max_value = PRINTER_IS_PRUSA_MINI() ? 11 : 3,
    .special_value = 0,
};

MI_SOUND_VOLUME::MI_SOUND_VOLUME()
    : WiSpin(static_cast<uint8_t>(Sound_GetVolume()), sound_volume_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_SOUND_VOLUME::OnClick() {
    Sound_SetVolume(GetVal());
}

/*****************************************************************************/
// MI_SORT_FILES
MI_SORT_FILES::MI_SORT_FILES()
    : WI_SWITCH_t<2>(config_store().file_sort.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_time), _(str_name)) {}
void MI_SORT_FILES::OnChange(size_t old_index) {
    if (old_index == WF_SORT_BY_TIME) { // default option - was sorted by time of change, set by name
        GuiFileSort::Set(WF_SORT_BY_NAME);
    } else if (old_index == WF_SORT_BY_NAME) { // was sorted by name, set by time
        GuiFileSort::Set(WF_SORT_BY_TIME);
    }
}

/*****************************************************************************/
// MI_TIMEZONE
static constexpr NumericInputConfig timezone_spin_config = {
    .min_value = -12,
    .max_value = 14,
    .unit = Unit::hour,
};

MI_TIMEZONE::MI_TIMEZONE()
    : WiSpin(config_store().timezone.get(), timezone_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEZONE::OnClick() {
    int8_t timezone = GetVal();
    config_store().timezone.set(timezone);
}

/*****************************************************************************/
// MI_TIMEZONE_MIN
MI_TIMEZONE_MIN::MI_TIMEZONE_MIN()
    : WI_SWITCH_t<3>(static_cast<uint8_t>(config_store().timezone_minutes.get()), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_0min), _(str_30min), _(str_45min)) {}

void MI_TIMEZONE_MIN::OnChange([[maybe_unused]] size_t old_index) {
    config_store().timezone_minutes.set(static_cast<time_tools::TimezoneOffsetMinutes>(index));
}

/*****************************************************************************/
// MI_TIMEZONE_SUMMER
MI_TIMEZONE_SUMMER::MI_TIMEZONE_SUMMER()
    : WI_ICON_SWITCH_OFF_ON_t(static_cast<uint8_t>(config_store().timezone_summer.get()), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_TIMEZONE_SUMMER::OnChange([[maybe_unused]] size_t old_index) {
    config_store().timezone_summer.set(static_cast<time_tools::TimezoneOffsetSummerTime>(index));
}

/*****************************************************************************/
// MI_TIME_FORMAT
MI_TIME_FORMAT::MI_TIME_FORMAT()
    : WI_SWITCH_t<2>(static_cast<uint8_t>(config_store().time_format.get()), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_12h), _(str_24h)) {}

void MI_TIME_FORMAT::OnChange([[maybe_unused]] size_t old_index) {
    config_store().time_format.set(static_cast<time_tools::TimeFormat>(index));
}

/*****************************************************************************/
// MI_TIME_NOW
MI_TIME_NOW::MI_TIME_NOW()
    : WI_SWITCH_t<1>(0, _(label), nullptr, is_enabled_t::no, is_hidden_t::no, string_view_utf8::MakeRAM((const uint8_t *)time_tools::get_time())) {
}

/*****************************************************************************/
// MI_FAN_CHECK
MI_FAN_CHECK::MI_FAN_CHECK()
    : WI_ICON_SWITCH_OFF_ON_t(bool(marlin_vars().fan_check_enabled), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_FAN_CHECK::OnChange(size_t old_index) {
    marlin_client::set_fan_check(!old_index);
    config_store().fan_check_enabled.set(static_cast<bool>(marlin_vars().fan_check_enabled));
}

MI_INFO_FW::MI_INFO_FW()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_INFO_FW::click([[maybe_unused]] IWindowMenu &window_menu) {
    // If we have development tools shown, click will print whole fw version string in info messagebox
    if constexpr (GuiDefaults::ShowDevelopmentTools) {
        MsgBoxInfo(string_view_utf8::MakeRAM((const uint8_t *)project_version_full), Responses_Ok);
    }
}

MI_INFO_BOOTLOADER::MI_INFO_BOOTLOADER()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_INFO_MMU::MI_INFO_MMU()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::yes) {
}

MI_INFO_BOARD::MI_INFO_BOARD()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_INFO_SERIAL_NUM::MI_INFO_SERIAL_NUM()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_FS_AUTOLOAD
static is_hidden_t get_autoload_hide_state() {
    // Autoloading option doesn't make sense with filament sensors disabled
    if (!config_store().fsensor_enabled.get()) {
        return is_hidden_t::yes;
    }

#if HAS_MMU2()
    // Do not show autoload option with MMU rework enabled - BFW-4290
    if (config_store().is_mmu_rework.get()) {
        return is_hidden_t::yes;
    }
#endif

    return is_hidden_t::no;
}

MI_FS_AUTOLOAD::MI_FS_AUTOLOAD()
    : WI_ICON_SWITCH_OFF_ON_t(bool(marlin_vars().fs_autoload_enabled), _(label), nullptr, is_enabled_t::yes, get_autoload_hide_state()) {}

void MI_FS_AUTOLOAD::OnChange(size_t old_index) {
    marlin_client::set_fs_autoload(!old_index);
    config_store().fs_autoload_enabled.set(static_cast<bool>(marlin_vars().fs_autoload_enabled));
}

/*****************************************************************************/
// MI_PRINT_PROGRESS_TIME
MI_PRINT_PROGRESS_TIME::MI_PRINT_PROGRESS_TIME()
    : WiSpin(config_store().print_progress_time.get(),
        config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_PRINT_PROGRESS_TIME::OnClick() {
    config_store().print_progress_time.set(GetVal());
}

/*****************************************************************************/
// MI_INFO_BED_TEMP
MI_INFO_BED_TEMP::MI_INFO_BED_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_INFO_FILL_SENSOR
MI_INFO_FILL_SENSOR::MI_INFO_FILL_SENSOR(const string_view_utf8 &label)
    : WI_LAMBDA_LABEL_t(
        label, nullptr, is_enabled_t::yes, is_hidden_t::no, [&](char *buffer) {
            static constexpr EnumArray<FilamentSensorState, const char *, 6> texts {
                { FilamentSensorState::NotInitialized, N_("uninitialized / %ld") },
                { FilamentSensorState::NotCalibrated, N_("uncalibrated / %ld") }, // not calibrated would be too long
                { FilamentSensorState::HasFilament, N_(" INS / %7ld") },
                { FilamentSensorState::NoFilament, N_("NINS / %7ld") },
                { FilamentSensorState::NotConnected, N_("disconnected / %ld") },
                { FilamentSensorState::Disabled, N_("disabled / %ld") },
            };

            StringViewUtf8Parameters<8> params;
            const auto orig_str = _(texts.get_fallback(state, FilamentSensorState::NotInitialized));
            orig_str.formatted(params, value).copyToRAM(buffer, GuiDefaults::infoDefaultLen);
        }) {}

void MI_INFO_FILL_SENSOR::UpdateValue(IFSensor *fsensor) {
    FilamentSensorState state = FilamentSensorState::NotInitialized;
    int32_t value = 0;
    if (fsensor) {
        state = fsensor->get_state();
        value = fsensor->GetFilteredValue();
    }
    if (this->state != state || this->value != value) {
        this->state = state;
        this->value = value;
        InValidateExtension();
    }
}

/*****************************************************************************/
// MI_INFO_PRINTER_FILL_SENSOR
MI_INFO_PRINTER_FILL_SENSOR::MI_INFO_PRINTER_FILL_SENSOR()
    : MI_INFO_FILL_SENSOR(_(label)) {}

/*****************************************************************************/
// MI_INFO_SIDE_FILL_SENSOR
MI_INFO_SIDE_FILL_SENSOR::MI_INFO_SIDE_FILL_SENSOR()
    : MI_INFO_FILL_SENSOR(_(label)) {}

/*****************************************************************************/
// MI_INFO_PRINT_FAN

MI_INFO_PRINT_FAN::MI_INFO_PRINT_FAN()
    : WI_FAN_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_INFO_HBR_FAN::MI_INFO_HBR_FAN()
    : WI_FAN_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_ODOMETER_DIST::MI_ODOMETER_DIST(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, float initVal)
    : WI_FORMATABLE_LABEL_t<float>(label, icon, enabled, hidden, initVal, [&](char *buffer) {
        float value_m = value / 1000; // change the unit from mm to m
        if (value_m > 999) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f km", (double)(value_m / 1000));
        } else {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f m", (double)value_m);
        }
    }) {
}

MI_ODOMETER_DIST_X::MI_ODOMETER_DIST_X()
    : MI_ODOMETER_DIST(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, -1) {
}
MI_ODOMETER_DIST_Y::MI_ODOMETER_DIST_Y()
    : MI_ODOMETER_DIST(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, -1) {
}
MI_ODOMETER_DIST_Z::MI_ODOMETER_DIST_Z()
    : MI_ODOMETER_DIST(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, -1) {
}

MI_ODOMETER_DIST_E::MI_ODOMETER_DIST_E()
    : MI_ODOMETER_DIST(_(generic_label), nullptr, is_enabled_t::yes, is_hidden_t::no, -1) {
}

MI_ODOMETER_MMU_CHANGES::MI_ODOMETER_MMU_CHANGES()
    : WI_FORMATABLE_LABEL_t<uint32_t>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {},
        [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%lu", value);
        }) {
}

MI_ODOMETER_TIME::MI_ODOMETER_TIME()
    : WI_FORMATABLE_LABEL_t<uint32_t>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, 0, [&](char *buffer) {
        format_duration(std::span { buffer, GuiDefaults::infoDefaultLen }, value);
    }) {}

MI_INFO_HEATER_VOLTAGE::MI_INFO_HEATER_VOLTAGE()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f V", (double)value);
        }) {
}

MI_INFO_INPUT_VOLTAGE::MI_INFO_INPUT_VOLTAGE()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f V", (double)value);
        }) {
}

MI_INFO_5V_VOLTAGE::MI_INFO_5V_VOLTAGE()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f V", (double)value);
        }) {
}

MI_INFO_HEATER_CURRENT::MI_INFO_HEATER_CURRENT()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f A", (double)value);
        }) {
}

MI_INFO_INPUT_CURRENT::MI_INFO_INPUT_CURRENT()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f A", (double)value);
        }) {
}

MI_INFO_MMU_CURRENT::MI_INFO_MMU_CURRENT()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f A", (double)value);
        }) {
}

MI_INFO_SPLITTER_5V_CURRENT::MI_INFO_SPLITTER_5V_CURRENT()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.2f A", (double)value);
        }) {
}

MI_INFO_SANDWICH_5V_CURRENT::MI_INFO_SANDWICH_5V_CURRENT()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.2f A", (double)value);
        }) {
}

MI_INFO_BUDDY_5V_CURRENT::MI_INFO_BUDDY_5V_CURRENT()
    : WI_FORMATABLE_LABEL_t<float>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%.2f A", (double)value);
        }) {
}

MI_INFO_BOARD_TEMP::MI_INFO_BOARD_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_INFO_MCU_TEMP::MI_INFO_MCU_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_FOOTER_RESET::MI_FOOTER_RESET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FOOTER_RESET::click([[maybe_unused]] IWindowMenu &window_menu) {
    // simple reset of footer eeprom would be better
    // but footer does not have reload method
    FooterItemHeater::ResetDrawMode();
    FooterLine::SetCenterN(footer::default_center_n_and_fewer);

    for (size_t i = 0; i < FOOTER_ITEMS_PER_LINE__; ++i) {
        config_store().set_footer_setting(i, footer::default_items[i]);
    }
    // send event for all footers
    Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::encode_item_for_event(footer::Item::none));

    // close this menu, because it is no longer valid and needs to be redrawn
    Screens::Access()->Close();
}

MI_HEATUP_BED::MI_HEATUP_BED()
    : WI_SWITCH_t<2>(config_store().heatup_bed.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(nozzle), _(nozzle_bed)) {
}
void MI_HEATUP_BED::OnChange(size_t old_index) {
    config_store().heatup_bed.set(!old_index);
}

MI_SET_READY::MI_SET_READY()
    : IWindowMenuItem(_(label), &img::set_ready_16x16, connect_client::MarlinPrinter::is_printer_ready() ? is_enabled_t::no : is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SET_READY::click([[maybe_unused]] IWindowMenu &window_menu) {
    if (connect_client::MarlinPrinter::set_printer_ready(true)) {
        Disable();
    }
}

#if HAS_PHASE_STEPPING()
MI_PHASE_STEPPING::MI_PHASE_STEPPING()
    : WI_ICON_SWITCH_OFF_ON_t(0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
    bool phstep_enabled = config_store().phase_stepping_enabled_x.get() || config_store().phase_stepping_enabled_y.get();
    set_value(phstep_enabled, false);
}

void MI_PHASE_STEPPING::OnChange([[maybe_unused]] size_t old_index) {
    if (event_in_progress) {
        return;
    }

    if (index && (config_store().selftest_result_phase_stepping.get() != TestResult_Passed)) {
    #if PRINTER_IS_PRUSA_iX()
        if (MsgBoxQuestion(_("Turn on Phase stepping uncalibrated?"), Responses_YesNo) == Response::No) {
            AutoRestore ar(event_in_progress, true);
            set_value(old_index, false);
            return;
        }
    #else
        AutoRestore ar(event_in_progress, true);
        MsgBoxWarning(_("Phase stepping not ready: perform calibration first."), Responses_Ok);
        set_value(old_index, false);
        return;
    #endif
    }

    if (index) {
        marlin_client::gcode("M970 X1 Y1"); // turn phase stepping on
    } else {
        marlin_client::gcode("M970 X0 Y0"); // turn phase stepping off
    }

    // we need to wait until the action actually takes place so that when returning
    // to the menu (if any) the new state is already reflected
    gui_dlg_wait([&]() {
        if (index == config_store().phase_stepping_enabled_x.get()) {
            Screens::Access()->Close();
        }
    });
}
#endif

#if HAS_COLDPULL()
MI_COLD_PULL::MI_COLD_PULL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_COLD_PULL::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("M1702");
}
#endif

MI_GCODE_VERIFY::MI_GCODE_VERIFY()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().verify_gcode.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_GCODE_VERIFY::OnChange([[maybe_unused]] size_t old_index) {
    bool newState = !config_store().verify_gcode.get();
    config_store().verify_gcode.set(newState);
}

/*****************************************************************************/
// MI_DEVHASH_IN_QR
MI_DEVHASH_IN_QR::MI_DEVHASH_IN_QR()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().devhash_in_qr.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_DEVHASH_IN_QR::OnChange(size_t old_index) {
    config_store().devhash_in_qr.set(!old_index);
}

#ifdef HAS_TMC_WAVETABLE
MI_WAVETABLE_XYZ::MI_WAVETABLE_XYZ()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().tmc_wavetable_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}
void MI_WAVETABLE_XYZ::OnChange(size_t old_index) {
    /// enable
    old_index ? tmc_disable_wavetable(true, true, true) : tmc_enable_wavetable(true, true, true);
    config_store().tmc_wavetable_enabled.set(!old_index);
}
#endif

/**********************************************************************************************/
// MI_LOAD_SETTINGS

MI_LOAD_SETTINGS::MI_LOAD_SETTINGS()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_LOAD_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    auto build_message = [](StringBuilder &msg_builder, const string_view_utf8 &name, bool ok) {
        msg_builder.append_string_view(name);
        msg_builder.append_string(": ");
        msg_builder.append_string_view(ok ? _("Ok") : _("Failed"));
        msg_builder.append_char('\n');
    };
    std::array<char, 150> msg;
    StringBuilder msg_builder(msg);
    msg_builder.append_string_view(_("\nLoading settings finished.\n\n"));

    const bool network_settings_loaded = netdev_load_ini_to_eeprom();
    if (network_settings_loaded) {
        notify_reconfigure();
    }
    build_message(msg_builder, _("Network"), network_settings_loaded);

#if BUDDY_ENABLE_CONNECT()
    build_message(msg_builder, _("Connect"), connect_client::MarlinPrinter::load_cfg_from_ini());
#endif

    MsgBoxInfo(string_view_utf8::MakeRAM((const uint8_t *)msg.data()), Responses_Ok);
}

/**********************************************************************************************/
// MI_USB_MSC_ENABLE
MI_USB_MSC_ENABLE::MI_USB_MSC_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().usb_msc_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_USB_MSC_ENABLE::OnChange(size_t old_index) {
    config_store().usb_msc_enabled.set(!old_index);
}
#if HAS_LEDS()
/**********************************************************************************************/
// MI_LEDS_ENABLE
MI_LEDS_ENABLE::MI_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(Animator_LCD_leds().animator_state(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_LEDS_ENABLE::OnChange(size_t old_index) {
    if (old_index) {
        Animator_LCD_leds().pause_animator();
    } else {
        Animator_LCD_leds().start_animator();
    }
}
#endif

#if HAS_SIDE_LEDS()
/**********************************************************************************************/
// MI_SIDE_LEDS_ENABLE
MI_SIDE_LEDS_ENABLE::MI_SIDE_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().side_leds_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_SIDE_LEDS_ENABLE::OnChange(size_t old_index) {
    leds::side_strip_control.SetEnable(!old_index);
    config_store().side_leds_enabled.set(!old_index);
}
#endif

#if HAS_SIDE_LEDS()
/**********************************************************************************************/
// MI_SIDE_LEDS_DIMMING
MI_SIDE_LEDS_DIMMING::MI_SIDE_LEDS_DIMMING()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().side_leds_dimming_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_SIDE_LEDS_DIMMING::OnChange(size_t) {
    config_store().side_leds_dimming_enabled.set(index);
    leds::side_strip_control.set_dimming_enabled(index);
}
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
/**********************************************************************************************/
// MI_TOOL_LEDS_ENABLE
MI_TOOL_LEDS_ENABLE::MI_TOOL_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().tool_leds_enabled.get(), _(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}
void MI_TOOL_LEDS_ENABLE::OnChange(size_t old_index) {
    HOTEND_LOOP() {
        prusa_toolchanger.getTool(e).set_cheese_led(!old_index ? 0xff : 0x00, 0x00);
    }
    config_store().tool_leds_enabled.set(!old_index);
}
#endif

// MI_TRIGGER_POWER_PANIC
MI_TRIGGER_POWER_PANIC::MI_TRIGGER_POWER_PANIC()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::no) {
}

void MI_TRIGGER_POWER_PANIC::click([[maybe_unused]] IWindowMenu &windowMenu) {
    // this is normally supposed to be called from ISR, but since disables IRQ so it works fine even outside of ISR
    power_panic::ac_fault_isr();
}

#if ENABLED(PRUSA_TOOLCHANGER)
/*****************************************************************************/
MI_PICK_PARK_TOOL::MI_PICK_PARK_TOOL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_PICK_PARK_TOOL::click(IWindowMenu & /*window_menu*/) {
    ToolActionBox<ToolBox::MenuPickPark>();
}

/*****************************************************************************/
MI_CALIBRATE_DOCK::MI_CALIBRATE_DOCK()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_CALIBRATE_DOCK::click(IWindowMenu & /*window_menu*/) {
    ToolActionBox<ToolBox::MenuCalibrateDock>();
    Screens::Access()->Get()->Validate();
}
#endif

/*****************************************************************************/
#if HAS_ILI9488_DISPLAY()
MI_DISPLAY_BAUDRATE::MI_DISPLAY_BAUDRATE()
    : WI_SWITCH_t(config_store().reduce_display_baudrate.get(), _("Display Refresh Speed"), nullptr, is_enabled_t::yes, is_hidden_t::no, _("High"), _("Low")) {
}

void MI_DISPLAY_BAUDRATE::OnChange(size_t) {
    config_store().reduce_display_baudrate.set(GetIndex());
}
#endif
