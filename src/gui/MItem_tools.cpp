#include "MItem_tools.hpp"
#include "png_resources.hpp"
#include "eeprom_loadsave.h"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "gui.hpp"
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
#include "DialogHandler.hpp"
#include "filament_sensor.hpp"
#include "main.h"
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "config.h"
#include "menu_spin_config.hpp"
#include "time_tools.hpp"
#include "footer_eeprom.hpp"
#include "version.h"
#include "../../common/PersistentStorage.h"
#include "sys.h"
#include "w25x.h"
#include <option/filament_sensor.h>
#include <crash_dump/dump.hpp>
#include <time.h>
#include "config_features.h"
#include <option/has_side_fsensor.h>
#include <configuration_store.hpp>
#include <option/bootloader.h>
#include <bootloader/bootloader.hpp>
#include "../../lib/Marlin/Marlin/src/feature/input_shaper/input_shaper_config.hpp"
#include "../../lib/Marlin/Marlin/src/feature/input_shaper/input_shaper.hpp"

static inline void MsgBoxNonBlockInfo(string_view_utf8 txt) {
    constexpr static const char *title = N_("Information");
    MsgBoxTitled mbt(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, txt, is_multiline::yes, _(title), &png::info_16x16);
    gui::TickLoop();
    gui_loop();
}

/**********************************************************************************************/
// MI_FILAMENT_SENSOR
bool MI_FILAMENT_SENSOR::init_index() const {
    return FSensors_instance().is_enabled();
}

void MI_FILAMENT_SENSOR::OnChange(size_t old_index) {
    if (old_index) {
        FSensors_instance().Disable();
    } else {
        FSensors_instance().Enable();

        // wait until it is initialized
        // no guiloop here !!! - it could cause show of unwanted error message
        FilamentSensors::EnableResult res;
        while ((res = FSensors_instance().get_enable_result()) == FilamentSensors::EnableResult::in_progress) {
            osDelay(0); // switch to other thread
        }

        // Check if sensor enabled successfully
        if (res != FilamentSensors::EnableResult::ok) {
            switch (res) {
            case FilamentSensors::EnableResult::not_calibrated:
                MsgBoxWarning(_("Filament sensor not ready: perform calibration first."), Responses_Ok);
                break;

            case FilamentSensors::EnableResult::not_connected:
                MsgBoxError(_("Filament sensor not connected, check wiring."), Responses_Ok);
                break;

            // Should not happen if sensors are working properly
            case FilamentSensors::EnableResult::disabled:
                MsgBoxError(_("Sensor logic error, printer filament sensor disabled."), Responses_Ok);
                break;

            // These cannot happen
            case FilamentSensors::EnableResult::ok:
            case FilamentSensors::EnableResult::in_progress:
                assert(false);
                break;
            }

            // Disable sensors again
            FSensors_instance().Disable();
            index = old_index;
            // wait until filament sensor command is processed
            // no guiloop here !!! - it could cause show of unwanted error message
            while (FSensors_instance().IsExtruderProcessingRequest()) {
                osDelay(0); // switch to other thread
            }
        }
    }
}

/*****************************************************************************/
// MI_LIVE_ADJUST_Z
MI_LIVE_ADJUST_Z::MI_LIVE_ADJUST_Z()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes,
#if PRINTER_IS_PRUSA_MINI
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
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_AUTO_HOME::click(IWindowMenu & /*window_menu*/) {
    marlin_event_clr(marlin_server::Event::CommandBegin);
    marlin_gcode("G28");
    while (!marlin_event_clr(marlin_server::Event::CommandBegin))
        marlin_client_loop();
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
}

/*****************************************************************************/
// MI_MESH_BED
MI_MESH_BED::MI_MESH_BED()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MESH_BED::click(IWindowMenu & /*window_menu*/) {
    Response response = Response::No;
    do {
        // home if we repeat MBL, nozzle may be in different position than expected
        if (!marlin_server::all_axes_homed() || response == Response::Yes) {
            marlin_event_clr(marlin_server::Event::CommandBegin);
            marlin_gcode("G28");
            while (!marlin_event_clr(marlin_server::Event::CommandBegin))
                marlin_client_loop();
            gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
        }
        response = Response::No;
        marlin_event_clr(marlin_server::Event::CommandBegin);
        marlin_gcode("G29");
        while (!marlin_event_clr(marlin_server::Event::CommandBegin))
            marlin_client_loop();
        gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);

        if (marlin_error(MARLIN_ERR_ProbingFailed)) {
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            response = MsgBox(_("Bed leveling failed. Try again?"), Responses_YesNo);
        }
    } while (response != Response::No);
}

/*****************************************************************************/
// MI_CALIB_Z

MI_CALIB_Z::MI_CALIB_Z()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_CALIB_Z::click(IWindowMenu & /*window_menu*/) {
    gui_dlg_calib_z();
}

/*****************************************************************************/
// MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_DISABLE_STEP::click(IWindowMenu & /*window_menu*/) {
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_MK3_5)
    marlin_gcode("M18 X Y E");
#else
    marlin_gcode("M18");
#endif
}

/*****************************************************************************/

namespace {
void do_factory_reset(bool wipe_fw) {
    auto msg = MsgBoxBase(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, wipe_fw ? _("Erasing everything,\nit will take some time...") : _("Erasing configuration,\nit will take some time..."));
    msg.Draw(); // Non-blocking info
    static constexpr uint32_t empty = 0xffffffff;
    for (uint16_t address = 0; address <= (8096 - 4); address += 4) {
        st25dv64k_user_write_bytes(address, &empty, sizeof(empty));
    }
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
    MsgBoxInfo(_("Reset complete. The system will now restart."), Responses_Ok);
    sys_reset();
}
} // anonymous namespace

MI_FACTORY_SOFT_RESET::MI_FACTORY_SOFT_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_SOFT_RESET::click(IWindowMenu & /*window_menu*/) {
    if (MsgBoxWarning(_("This operation cannot be undone. Current user configuration and passwords will be lost!\nDo you want to reset the printer to factory defaults?"), Responses_YesNo, 1) == Response::Yes) {
        do_factory_reset(false);
    }
}

MI_FACTORY_HARD_RESET::MI_FACTORY_HARD_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_HARD_RESET::click(IWindowMenu & /*window_menu*/) {
    static constexpr char fmt2Translate[] = N_("This operation cannot be undone. Current configuration will be lost!\nYou will need a USB drive with this firmware (%s_firmware_%s.bbf file) to start the printer again.\nDo you really want to continue?");
    char buff[sizeof(fmt2Translate) + 20]; // expecting xx.xx.xx for version (8) + max 5 for for PRINTER_MODEL + some reserve
    {
        char translated_fmt[sizeof(buff)];
        _(fmt2Translate).copyToRAM(translated_fmt, sizeof(translated_fmt));
        snprintf(buff, sizeof(buff), translated_fmt, PRINTER_MODEL, project_version);
    }

    if (MsgBoxWarning(_(buff), Responses_YesNo, 1) == Response::Yes) {
        do_factory_reset(true);
    }
}

/*****************************************************************************/
// MI_ENTER_DFU
#ifdef BUDDY_ENABLE_DFU_ENTRY
MI_ENTER_DFU::MI_ENTER_DFU()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_ENTER_DFU::click(IWindowMenu &) {
    sys_dfu_request_and_reset();
}
#endif

/*****************************************************************************/
// MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SAVE_DUMP::click(IWindowMenu & /*window_menu*/) {
    MsgBoxNonBlockInfo(_("A crash dump is being saved."));
    if (crash_dump::dump_save_to_usb("/usb/dump.bin"))
        MsgBoxInfo(_("A crash dump report (file dump.bin) has been saved to the USB drive."), Responses_Ok);
    else
        MsgBoxError(_("Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again."), Responses_Ok);
}

/*****************************************************************************/
// MI_XFLASH_RESET
MI_XFLASH_RESET::MI_XFLASH_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_XFLASH_RESET::click(IWindowMenu & /*window_menu*/) {
    crash_dump::dump_in_xflash_reset();
}

/*****************************************************************************/
// MI_HF_TEST_0
MI_HF_TEST_0::MI_HF_TEST_0()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_HF_TEST_0::click(IWindowMenu & /*window_menu*/) {
    crash_dump::dump_hardfault_test_0();
}

/*****************************************************************************/
// MI_HF_TEST_1
MI_HF_TEST_1::MI_HF_TEST_1()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_HF_TEST_1::click(IWindowMenu & /*window_menu*/) {
    crash_dump::dump_hardfault_test_1();
}

/*****************************************************************************/
// MI_EE_LOAD_400
MI_EE_LOAD_400::MI_EE_LOAD_400()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_400::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.0-final+1965.bin");
    sys_reset();
}

/*****************************************************************************/
// MI_EE_LOAD_401
MI_EE_LOAD_401::MI_EE_LOAD_401()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_401::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.1-final+1974.bin");
    sys_reset();
}

/*****************************************************************************/
// MI_EE_LOAD_402
MI_EE_LOAD_402::MI_EE_LOAD_402()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_402::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.2-final+1977.bin");
    sys_reset();
}

/*****************************************************************************/
// MI_EE_LOAD_403RC1
MI_EE_LOAD_403RC1::MI_EE_LOAD_403RC1()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_403RC1::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.3-RC1+246.bin");
    sys_reset();
}

/*****************************************************************************/
// MI_EE_LOAD_403
MI_EE_LOAD_403::MI_EE_LOAD_403()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_403::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.3-final+258.bin");
    sys_reset();
}

/*****************************************************************************/
// MI_EE_LOAD
MI_EE_LOAD::MI_EE_LOAD()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom.bin");
    sys_reset();
}

/*****************************************************************************/
// MI_EE_SAVE
MI_EE_SAVE::MI_EE_SAVE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_SAVE::click(IWindowMenu & /*window_menu*/) {
    eeprom_save_bin_to_usb("/usb/eeprom.bin");
}

/*****************************************************************************/
// MI_EE_SAVEXML
MI_EE_SAVEXML::MI_EE_SAVEXML()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_SAVEXML::click(IWindowMenu & /*window_menu*/) {
    // eeprom_save_xml_to_usb("/usb/eeprom.xml"); // TODO(ConfigStore): Not yet migrated
}

/*****************************************************************************/
// MI_EE_CLEAR
MI_EE_CLEAR::MI_EE_CLEAR()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
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
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_M600::click(IWindowMenu & /*window_menu*/) {
    marlin_gcode_push_front("M600");
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
    return (size_t)(sound_mode > eSOUND_MODE::ASSIST ? eSOUND_MODE::DEFAULT_SOUND : sound_mode);
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
MI_SOUND_VOLUME::MI_SOUND_VOLUME()
    : WiSpinInt(static_cast<uint8_t>(Sound_GetVolume()), SpinCnf::volume_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_SOUND_VOLUME::OnClick() {
    Sound_SetVolume(GetVal());
}

/*****************************************************************************/
// MI_SORT_FILES
MI_SORT_FILES::MI_SORT_FILES()
    : WI_SWITCH_t<2>(config_store().file_sort.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_time), _(str_name)) {}
void MI_SORT_FILES::OnChange(size_t old_index) {
    if (old_index == WF_SORT_BY_TIME) {        // default option - was sorted by time of change, set by name
        GuiFileSort::Set(WF_SORT_BY_NAME);
    } else if (old_index == WF_SORT_BY_NAME) { // was sorted by name, set by time
        GuiFileSort::Set(WF_SORT_BY_TIME);
    }
}

/*****************************************************************************/
// MI_TIMEZONE
MI_TIMEZONE::MI_TIMEZONE()
    : WiSpinInt(config_store().timezone.get(), SpinCnf::timezone_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEZONE::OnClick() {
    int8_t timezone = GetVal();
    config_store().timezone.set(timezone);
}

/*****************************************************************************/
// MI_TIME_FORMAT
MI_TIME_FORMAT::MI_TIME_FORMAT()
    : WI_SWITCH_t<2>(static_cast<uint8_t>(config_store().time_format.get()), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_12h), _(str_24h)) {}

void MI_TIME_FORMAT::OnChange(size_t old_index) {
    if (old_index == (size_t)time_format::TF_t::TF_12H) {        // default option - 12h time format
        time_format::Change(time_format::TF_t::TF_24H);
    } else if (old_index == (size_t)time_format::TF_t::TF_24H) { // 24h time format
        time_format::Change(time_format::TF_t::TF_12H);
    }
}

/*****************************************************************************/
// IMI_FS_SPAN
IMI_FS_SPAN::IMI_FS_SPAN(bool is_side_, size_t index_, const char *label)
    : WiSpinInt(is_side_ ? config_store().get_side_fs_value_span(index_) : config_store().get_extruder_fs_value_span(index_), SpinCnf::fs_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev)
    , is_side(is_side_)
    , index(index_) {}

void IMI_FS_SPAN::OnClick() {
    if (is_side) {
        config_store().set_side_fs_value_span(index, GetVal());
    } else {
        config_store().set_extruder_fs_value_span(index, GetVal());
    }
}

/*****************************************************************************/
// IMI_FS_REF
// in case we don't want to allow modification just change is_enabled_t to yes
IMI_FS_REF::IMI_FS_REF(bool is_side_, size_t index_, const char *label)
    : WiSpinInt(is_side_ ? config_store().get_side_fs_ref_value(index_) : config_store().get_extruder_fs_ref_value(index_), SpinCnf::int_num, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev)
    , is_side(is_side_)
    , index(index_) {}

void IMI_FS_REF::OnClick() {
    if (is_side) {
        config_store().set_side_fs_ref_value(index, GetVal());
    } else {
        config_store().set_extruder_fs_ref_value(index, GetVal());
    }
}

/*****************************************************************************/
// MI_FAN_CHECK
MI_FAN_CHECK::MI_FAN_CHECK()
    : WI_ICON_SWITCH_OFF_ON_t(bool(marlin_vars()->fan_check_enabled), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_FAN_CHECK::OnChange(size_t old_index) {
    marlin_set_fan_check(!old_index);
    config_store().fan_check_enabled.set(static_cast<bool>(marlin_vars()->fan_check_enabled));
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

MI_INFO_BOARD::MI_INFO_BOARD()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_INFO_SERIAL_NUM::MI_INFO_SERIAL_NUM()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_METRICS_ENABLE
MI_METRICS_ENABLE::MI_METRICS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().metrics_allow.get() == MetricsAllow::All, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_METRICS_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    if (index) { // Enable
        if (MsgBoxWarning(_(txt_confirm), Responses_YesNo, 1) == Response::Yes) {
            config_store().metrics_allow.set(MetricsAllow::All);
        } else {
            index = false; // User changed his mind
        }
    } else {
        config_store().metrics_allow.set(MetricsAllow::None);
    }
}

/*****************************************************************************/
// MI_FS_AUTOLOAD
is_hidden_t hide_autoload_item() {
    return FSensors_instance().GetAutoload() == fsensor_t::Disabled ? is_hidden_t::yes : is_hidden_t::no;
}

MI_FS_AUTOLOAD::MI_FS_AUTOLOAD()
    : WI_ICON_SWITCH_OFF_ON_t(bool(marlin_vars()->fs_autoload_enabled), _(label), nullptr, is_enabled_t::yes, hide_autoload_item()) {}
void MI_FS_AUTOLOAD::OnChange(size_t old_index) {
    marlin_set_fs_autoload(!old_index);
    config_store().fs_autoload_enabled.set(static_cast<bool>(marlin_vars()->fs_autoload_enabled));
}

/*****************************************************************************/
// MI_PRINT_PROGRESS_TIME
MI_PRINT_PROGRESS_TIME::MI_PRINT_PROGRESS_TIME()
    : WiSpinInt(config_store().print_progress_time.get(),
        SpinCnf::print_progress, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
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
MI_INFO_FILL_SENSOR::MI_INFO_FILL_SENSOR(string_view_utf8 label)
    : WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>>(
        label, nullptr, is_enabled_t::yes, is_hidden_t::no, { {}, {} }, [&](char *buffer) {
            if (value.second.attribute.valid || value.first.attribute.valid) {

                static constexpr char disconnected[] = N_("disconnected / %ld");
                static constexpr char notCalibrated[] = N_("uncalibrated / %ld"); // not calibrated would be too long
                static constexpr char inserted[] = N_(" INS / %7ld");
                static constexpr char notInserted[] = N_("NINS / %7ld");
                static constexpr char disabled[] = N_("disabled / %ld");
                static constexpr char notInitialized[] = N_("uninitialized / %ld");

                char fmt[GuiDefaults::infoDefaultLen]; // max len of extension
                switch ((fsensor_t)value.first.int_val) {
                case fsensor_t::NotInitialized:
                    _(notInitialized).copyToRAM(fmt, sizeof(fmt));
                    break;
                case fsensor_t::NotConnected:
                    _(disconnected).copyToRAM(fmt, sizeof(fmt));
                    break;
                case fsensor_t::Disabled:
                    _(disabled).copyToRAM(fmt, sizeof(fmt));
                    break;
                case fsensor_t::NotCalibrated:
                    _(notCalibrated).copyToRAM(fmt, sizeof(fmt));
                    break;
                case fsensor_t::HasFilament:
                    _(inserted).copyToRAM(fmt, sizeof(fmt));
                    break;
                case fsensor_t::NoFilament:
                    _(notInserted).copyToRAM(fmt, sizeof(fmt));
                    break;
                }
                snprintf(buffer, GuiDefaults::infoDefaultLen, fmt, value.second.int_val);
            } else {
                if (value.first.attribute.valid || value.second.attribute.valid) {
                    strlcpy(buffer, NA, GuiDefaults::infoDefaultLen);
                } else {
                    strlcpy(buffer, NI, GuiDefaults::infoDefaultLen);
                }
            }
        }) {}

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

MI_ODOMETER_DIST::MI_ODOMETER_DIST(string_view_utf8 label, const png::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, float initVal)
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

MI_ODOMETER_TIME::MI_ODOMETER_TIME()
    : WI_FORMATABLE_LABEL_t<uint32_t>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, 0, [&](char *buffer) {
        time_t time = (time_t)value;
        constexpr static uint32_t secPerDay = 24 * 60 * 60;
        const struct tm *timeinfo = localtime(&time);
        if (timeinfo->tm_yday) {
            // days are recalculated, because timeinfo shows number of days in year and we want more days than 365
            uint16_t days = value / secPerDay;
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%ud %uh", days, timeinfo->tm_hour);
        } else if (timeinfo->tm_hour) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
        } else if (timeinfo->tm_min) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%im %2is", timeinfo->tm_min, timeinfo->tm_sec);
        } else {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%is", timeinfo->tm_sec);
        }
    }) {}

MI_INFO_HEATER_VOLTAGE::MI_INFO_HEATER_VOLTAGE()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f V", (double)value.float_val);
            }
        }) {
}

MI_INFO_INPUT_VOLTAGE::MI_INFO_INPUT_VOLTAGE()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f V", (double)value.float_val);
            }
        }) {
}

MI_INFO_5V_VOLTAGE::MI_INFO_5V_VOLTAGE()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f V", (double)value.float_val);
            }
        }) {
}

MI_INFO_HEATER_CURRENT::MI_INFO_HEATER_CURRENT()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f A", (double)value.float_val);
            }
        }) {
}

MI_INFO_INPUT_CURRENT::MI_INFO_INPUT_CURRENT()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f A", (double)value.float_val);
            }
        }) {
}

MI_INFO_MMU_CURRENT::MI_INFO_MMU_CURRENT()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f A", (double)value.float_val);
            }
        }) {
}

MI_INFO_SPLITTER_5V_CURRENT::MI_INFO_SPLITTER_5V_CURRENT()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.2f A", (double)value.float_val);
            }
        }) {
}

MI_INFO_SANDWICH_5V_CURRENT::MI_INFO_SANDWICH_5V_CURRENT()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.2f A", (double)value.float_val);
            }
        }) {
}

MI_INFO_BUDDY_5V_CURRENT::MI_INFO_BUDDY_5V_CURRENT()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.2f A", (double)value.float_val);
            }
        }) {
}

MI_INFO_BOARD_TEMP::MI_INFO_BOARD_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_FOOTER_RESET::MI_FOOTER_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FOOTER_RESET::click([[maybe_unused]] IWindowMenu &window_menu) {
    // simple reset of footer eeprom would be better
    // but footer does not have reload method
    FooterItemHeater::ResetDrawMode();
    FooterLine::SetCenterN(footer::DefaultCenterNAndFewer);

    footer::eeprom::Store(footer::DefaultItems);
    // send event for all footers
    Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::EncodeItemForEvent(footer::Item::None));

    // close this menu, because it is no longer valid and needs to be redrawn
    Screens::Access()->Close();
}

MI_HEATUP_BED::MI_HEATUP_BED()
    : WI_SWITCH_t<2>(config_store().heatup_bed.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(nozzle), _(nozzle_bed)) {
}
void MI_HEATUP_BED::OnChange(size_t old_index) {
    config_store().heatup_bed.set(!old_index);
}

MI_INFO_SERIAL_NUM_XLCD::MI_INFO_SERIAL_NUM_XLCD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

/*****************************************************************************/
// INPUT SHAPER

static bool input_shaper_x_enabled() {
#if PRINTER_IS_PRUSA_MK4
    return config_store().input_shaper_axis_x_enabled.get();
#else
    return false; // TODO(InputShaper)
#endif
}

static bool input_shaper_y_enabled() {
#if PRINTER_IS_PRUSA_MK4
    return config_store().input_shaper_axis_y_enabled.get();
#else
    return false; // TODO(InputShaper)
#endif
}

static int32_t input_shaper_x_type() {
    const auto axis_x = config_store().input_shaper_axis_x_config.get();
    return static_cast<int32_t>(axis_x.type);
}

static int32_t input_shaper_y_type() {
    const auto axis_y = config_store().input_shaper_axis_y_config.get();
    return static_cast<int32_t>(axis_y.type);
}

static uint32_t input_shaper_x_frequency() {
    const auto axis_x = config_store().input_shaper_axis_x_config.get();
    return static_cast<int32_t>(axis_x.frequency);
}

static uint32_t input_shaper_y_frequency() {
    const auto axis_y = config_store().input_shaper_axis_y_config.get();
    return static_cast<int32_t>(axis_y.frequency);
}

static bool input_shaper_y_weight_compensation() {
    return config_store().input_shaper_weight_adjust_y_enabled.get();
}

MI_IS_X_ONOFF::MI_IS_X_ONOFF()
    : WI_ICON_SWITCH_OFF_ON_t(input_shaper_x_enabled(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_IS_X_ONOFF::OnChange(size_t) {
    config_store().input_shaper_axis_x_enabled.set(index);
    if (index) {
        input_shaper::set_axis_config(X_AXIS, config_store().input_shaper_axis_x_config.get());
    } else {
        input_shaper::set_axis_config(X_AXIS, std::nullopt);
    }
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)&param);
}

MI_IS_Y_ONOFF::MI_IS_Y_ONOFF()
    : WI_ICON_SWITCH_OFF_ON_t(input_shaper_y_enabled(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_IS_Y_ONOFF::OnChange(size_t) {
    config_store().input_shaper_axis_y_enabled.set(index);
    if (index) {
        input_shaper::set_axis_config(Y_AXIS, config_store().input_shaper_axis_y_config.get());
    } else {
        input_shaper::set_axis_config(Y_AXIS, std::nullopt);
    }
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)&param);
}

MI_IS_X_TYPE::MI_IS_X_TYPE()
    // clang-format off
    : WI_SWITCH_t<6>(input_shaper_x_type(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zvd))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::mzv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_2hump))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_3hump))
    ) {
    // clang-format on
    if (!input_shaper_x_enabled())
        DontShowDisabledExtension();
}

void MI_IS_X_TYPE::OnChange(size_t) {
    auto axis_x = config_store().input_shaper_axis_x_config.get();
    axis_x.type = static_cast<input_shaper::Type>(GetIndex());
    config_store().input_shaper_axis_x_config.set(axis_x);
    input_shaper::set_axis_config(X_AXIS, axis_x);
}

MI_IS_Y_TYPE::MI_IS_Y_TYPE()
    // clang-format off
    : WI_SWITCH_t<6>(input_shaper_y_type(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::zvd))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::mzv))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_2hump))
    , string_view_utf8::MakeCPUFLASH((const uint8_t *)input_shaper::to_string(input_shaper::Type::ei_3hump))
    ) {
    // clang-format on
    if (!input_shaper_y_enabled())
        DontShowDisabledExtension();
}

void MI_IS_Y_TYPE::OnChange(size_t) {
    auto axis_y = config_store().input_shaper_axis_y_config.get();
    axis_y.type = static_cast<input_shaper::Type>(GetIndex());
    config_store().input_shaper_axis_y_config.set(axis_y);
    input_shaper::set_axis_config(Y_AXIS, axis_y);
}

static constexpr SpinConfigInt is_frequency_spin_config = makeSpinConfig<int>(
    { static_cast<int>(input_shaper::frequency_safe_min), static_cast<int>(input_shaper::frequency_safe_max), 1 },
    "Hz",
    spin_off_opt_t::no);

MI_IS_X_FREQUENCY::MI_IS_X_FREQUENCY()
    : WiSpinInt(input_shaper_x_frequency(), is_frequency_spin_config, _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
    if (!input_shaper_x_enabled())
        DontShowDisabledExtension();
}

void MI_IS_X_FREQUENCY::OnClick() {
    auto axis_x = config_store().input_shaper_axis_x_config.get();
    axis_x.frequency = static_cast<float>(GetVal());
    config_store().input_shaper_axis_x_config.set(axis_x);
    input_shaper::set_axis_config(X_AXIS, axis_x);
}

MI_IS_Y_FREQUENCY::MI_IS_Y_FREQUENCY()
    : WiSpinInt(input_shaper_y_frequency(), is_frequency_spin_config, _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
    if (!input_shaper_y_enabled())
        DontShowDisabledExtension();
}

void MI_IS_Y_FREQUENCY::OnClick() {
    auto axis_y = config_store().input_shaper_axis_y_config.get();
    axis_y.frequency = static_cast<float>(GetVal());
    config_store().input_shaper_axis_y_config.set(axis_y);
    input_shaper::set_axis_config(Y_AXIS, axis_y);
}

MI_IS_Y_COMPENSATION::MI_IS_Y_COMPENSATION()
    : WI_ICON_SWITCH_OFF_ON_t(input_shaper_y_weight_compensation(), _(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {
    if (!input_shaper_y_enabled())
        DontShowDisabledExtension();
}

void MI_IS_Y_COMPENSATION::OnChange(size_t) {
    config_store().input_shaper_weight_adjust_y_enabled.set(index);
    if (index) {
        input_shaper::current_config().weight_adjust_y = config_store().input_shaper_weight_adjust_y_config.get();
    } else {
        input_shaper::current_config().weight_adjust_y = std::nullopt;
    }
}

MI_IS_SET::MI_IS_SET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_IS_SET::click(IWindowMenu &) {
    MsgBoxISWarning(_("ATTENTION: Changing any Input Shaper values will overwrite them permanently. To revert to a stock setup, visit prusa.io/input-shaper or run a factory reset."), Responses_Ok);
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)&param);
}

MI_IS_CALIB::MI_IS_CALIB()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_IS_CALIB::click([[maybe_unused]] IWindowMenu &window_menu) {
    // TODO(InputShaper)
}
