#include "MItem_tools.hpp"
#include "png_resources.hpp"
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "marlin_client.hpp"
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
#include "loadcell.h"
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
#include <crash_dump/dump.h>
#include <time.h>
#include "config_features.h"
#include <option/has_side_fsensor.h>

#if HAS_TOOLCHANGER()
    #include <puppies/Dwarf.hpp>
    #include "src/module/prusa/toolchanger.h"
#endif

static inline void MsgBoxNonBlockInfo(string_view_utf8 txt) {
    constexpr static const char *title = N_("Information");
    MsgBoxTitled mbt(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, txt, is_multiline::yes, _(title), &png::info_16x16);
    gui::TickLoop();
    gui_loop();
}

/**********************************************************************************************/
//MI_FILAMENT_SENSOR
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
#if PRINTER_TYPE == PRINTER_PRUSA_MINI
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
//MI_AUTO_HOME
MI_AUTO_HOME::MI_AUTO_HOME()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_AUTO_HOME::click(IWindowMenu & /*window_menu*/) {
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    marlin_gcode("G28");
    while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
        marlin_client_loop();
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
}

/*****************************************************************************/
//MI_MESH_BED
MI_MESH_BED::MI_MESH_BED()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MESH_BED::click(IWindowMenu & /*window_menu*/) {
    Response response = Response::No;
    do {
        //home if we repeat MBL, nozzle may be in different position than expected
        if (!marlin_all_axes_homed() || response == Response::Yes) {
            marlin_event_clr(MARLIN_EVT_CommandBegin);
            marlin_gcode("G28");
            while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                marlin_client_loop();
            gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
        }
        response = Response::No;
        marlin_event_clr(MARLIN_EVT_CommandBegin);
        marlin_gcode("G29");
        while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
            marlin_client_loop();
        gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);

        if (marlin_error(MARLIN_ERR_ProbingFailed)) {
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            response = MsgBox(_("Bed leveling failed. Try again?"), Responses_YesNo);
        }
    } while (response != Response::No);
}

/*****************************************************************************/
//MI_CALIB_Z

MI_CALIB_Z::MI_CALIB_Z()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_CALIB_Z::click(IWindowMenu & /*window_menu*/) {
    gui_dlg_calib_z();
}

/*****************************************************************************/
//MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_DISABLE_STEP::click(IWindowMenu & /*window_menu*/) {
#if (PRINTER_TYPE == PRINTER_PRUSA_MK404 || PRINTER_TYPE == PRINTER_PRUSA_XL)
    marlin_gcode("M18 X Y E");
#else
    marlin_gcode("M18");
#endif
}

/*****************************************************************************/
//MI_FACTORY_DEFAULTS
MI_FACTORY_DEFAULTS::MI_FACTORY_DEFAULTS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_DEFAULTS::click(IWindowMenu & /*window_menu*/) {
    if (MsgBoxWarning(_("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?"), Responses_YesNo, 1) == Response::Yes) {
        PersistentStorage::erase();
        eeprom_defaults();
        MsgBoxInfo(_("Factory defaults loaded. The system will now restart."), Responses_Ok);
        sys_reset();
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
//MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SAVE_DUMP::click(IWindowMenu & /*window_menu*/) {
    MsgBoxNonBlockInfo(_("A crash dump is being saved."));
    if (dump_save_to_usb("/usb/dump.bin"))
        MsgBoxInfo(_("A crash dump report (file dump.bin) has been saved to the USB drive."), Responses_Ok);
    else
        MsgBoxError(_("Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again."), Responses_Ok);
}

/*****************************************************************************/
//MI_XFLASH_DELETE
MI_XFLASH_DELETE::MI_XFLASH_DELETE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_XFLASH_DELETE::click(IWindowMenu & /*window_menu*/) {
    auto res = MsgBoxWarning(_("Do you want to erase the external flash? The system will restart when complete."), Responses_YesNo);
    if (res == Response::Yes) {
        w25x_chip_erase();
        sys_reset();
    }
}

/*****************************************************************************/
//MI_XFLASH_RESET
MI_XFLASH_RESET::MI_XFLASH_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_XFLASH_RESET::click(IWindowMenu & /*window_menu*/) {
    dump_in_xflash_reset();
}

/*****************************************************************************/
//MI_HF_TEST_0
MI_HF_TEST_0::MI_HF_TEST_0()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_HF_TEST_0::click(IWindowMenu & /*window_menu*/) {
    dump_hardfault_test_0();
}

/*****************************************************************************/
//MI_HF_TEST_1
MI_HF_TEST_1::MI_HF_TEST_1()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_HF_TEST_1::click(IWindowMenu & /*window_menu*/) {
    dump_hardfault_test_1();
}

/*****************************************************************************/
//MI_EE_LOAD_400
MI_EE_LOAD_400::MI_EE_LOAD_400()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_400::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.0-final+1965.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_401
MI_EE_LOAD_401::MI_EE_LOAD_401()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_401::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.1-final+1974.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_402
MI_EE_LOAD_402::MI_EE_LOAD_402()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_402::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.2-final+1977.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403RC1
MI_EE_LOAD_403RC1::MI_EE_LOAD_403RC1()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_403RC1::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.3-RC1+246.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403
MI_EE_LOAD_403::MI_EE_LOAD_403()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_403::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom/eeprom_MINI-4.0.3-final+258.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD
MI_EE_LOAD::MI_EE_LOAD()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("/usb/eeprom.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_SAVE
MI_EE_SAVE::MI_EE_SAVE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_SAVE::click(IWindowMenu & /*window_menu*/) {
    eeprom_save_bin_to_usb("/usb/eeprom.bin");
}

/*****************************************************************************/
//MI_EE_SAVEXML
MI_EE_SAVEXML::MI_EE_SAVEXML()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_SAVEXML::click(IWindowMenu & /*window_menu*/) {
    eeprom_save_xml_to_usb("/usb/eeprom.xml");
}

/*****************************************************************************/
//MI_EE_CLEAR
MI_EE_CLEAR::MI_EE_CLEAR()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_EE_CLEAR::click(IWindowMenu & /*window_menu*/) {
    eeprom_clear();
}

/*****************************************************************************/
//MI_M600
MI_M600::MI_M600()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_M600::click(IWindowMenu & /*window_menu*/) {
    marlin_gcode_push_front("M600");
}

/*****************************************************************************/
//MI_TIMEOUT
MI_TIMEOUT::MI_TIMEOUT()
    : WI_ICON_SWITCH_OFF_ON_t(Screens::Access()->GetMenuTimeout() ? 1 : 0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEOUT::OnChange(size_t old_index) {
    if (!old_index) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }
    eeprom_set_bool(EEVAR_MENU_TIMEOUT, uint8_t(Screens::Access()->GetMenuTimeout()));
}

/*****************************************************************************/
//MI_SOUND_MODE
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
//MI_SOUND_TYPE
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
//MI_SOUND_VOLUME
MI_SOUND_VOLUME::MI_SOUND_VOLUME()
    : WiSpinInt(static_cast<uint8_t>(Sound_GetVolume()), SpinCnf::volume_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_SOUND_VOLUME::OnClick() {
    Sound_SetVolume(GetVal());
}

/*****************************************************************************/
//MI_SORT_FILES
MI_SORT_FILES::MI_SORT_FILES()
    : WI_SWITCH_t<2>(eeprom_get_ui8(EEVAR_FILE_SORT), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_time), _(str_name)) {}
void MI_SORT_FILES::OnChange(size_t old_index) {
    if (old_index == WF_SORT_BY_TIME) { // default option - was sorted by time of change, set by name
        GuiFileSort::Set(WF_SORT_BY_NAME);
    } else if (old_index == WF_SORT_BY_NAME) { // was sorted by name, set by time
        GuiFileSort::Set(WF_SORT_BY_TIME);
    }
}

/*****************************************************************************/
//MI_TIMEZONE
MI_TIMEZONE::MI_TIMEZONE()
    : WiSpinInt(eeprom_get_i8(EEVAR_TIMEZONE), SpinCnf::timezone_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEZONE::OnClick() {
    int8_t timezone = GetVal();
    eeprom_set_i8(EEVAR_TIMEZONE, timezone);
}

/*****************************************************************************/
//MI_TIME_FORMAT
MI_TIME_FORMAT::MI_TIME_FORMAT()
    : WI_SWITCH_t<2>(eeprom_get_ui8(EEVAR_TIME_FORMAT), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_12h), _(str_24h)) {}

void MI_TIME_FORMAT::OnChange(size_t old_index) {
    if (old_index == (size_t)time_format::TF_t::TF_12H) { // default option - 12h time format
        time_format::Change(time_format::TF_t::TF_24H);
    } else if (old_index == (size_t)time_format::TF_t::TF_24H) { // 24h time format
        time_format::Change(time_format::TF_t::TF_12H);
    }
}

/*****************************************************************************/
//MI_LOADCELL_SCALE
MI_LOADCELL_SCALE::MI_LOADCELL_SCALE()
    : WiSpinInt((int)(variant8_get_flt(eeprom_get_var(EEVAR_LOADCELL_SCALE)) * 1000), SpinCnf::loadcell_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}
void MI_LOADCELL_SCALE::OnClick() {
    float scale = (float)GetVal() / 1000;
    loadcell.SetScale(scale);
    eeprom_set_var(EEVAR_LOADCELL_SCALE, variant8_flt(scale));
}

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
MI_FILAMENT_SENSOR_STATE::MI_FILAMENT_SENSOR_STATE()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_FILAMENT_SENSOR_STATE::state_t MI_FILAMENT_SENSOR_STATE::get_state() {
    fsensor_t fs = FSensors_instance().GetPrimaryRunout();
    switch (fs) {
    case fsensor_t::HasFilament:
        return state_t::high;
    case fsensor_t::NoFilament:
        return state_t::low;
    default:;
    }
    return state_t::unknown;
}

void MI_FILAMENT_SENSOR_STATE::Loop() {
    SetIndex((size_t)get_state());
}

MI_MINDA::MI_MINDA()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_MINDA::state_t MI_MINDA::get_state() {
    return (buddy::hw::zMin.read() == buddy::hw::Pin::State::low) ? state_t::low : state_t::high;
}

void MI_MINDA::Loop() {
    SetIndex((size_t)get_state());
}
#endif

#include "filament_sensors_handler.hpp"

/*****************************************************************************/
//IMI_FS_SPAN
IMI_FS_SPAN::IMI_FS_SPAN(eevar_id eevar, size_t index, const char *label)
    : WiSpinInt((int)(eeprom_get_var(eevar)), SpinCnf::fs_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev)
    , eevar(eevar)
    , index(index) {}

void IMI_FS_SPAN::OnClick() {
    eeprom_set_var(eevar, variant8_ui32(GetVal()));
    GetExtruderFSensor(index);
}

/*****************************************************************************/
//MI_FAN_CHECK
MI_FAN_CHECK::MI_FAN_CHECK()
    : WI_ICON_SWITCH_OFF_ON_t(bool(marlin_vars()->fan_check_enabled), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_FAN_CHECK::OnChange(size_t old_index) {
    marlin_set_fan_check(!old_index);
    eeprom_set_bool(EEVAR_FAN_CHECK_ENABLED, bool(marlin_vars()->fan_check_enabled));
}

MI_INFO_FW::MI_INFO_FW()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_INFO_FW::click(IWindowMenu &window_menu) {
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
//MI_FS_AUTOLOAD
is_hidden_t hide_autoload_item() {
    return FSensors_instance().GetAutoload() == fsensor_t::Disabled ? is_hidden_t::yes : is_hidden_t::no;
}

MI_FS_AUTOLOAD::MI_FS_AUTOLOAD()
    : WI_ICON_SWITCH_OFF_ON_t(bool(marlin_vars()->fs_autoload_enabled), _(label), nullptr, is_enabled_t::yes, hide_autoload_item()) {}
void MI_FS_AUTOLOAD::OnChange(size_t old_index) {
    marlin_set_fs_autoload(!old_index);
    eeprom_set_bool(EEVAR_FS_AUTOLOAD_ENABLED, bool(marlin_vars()->fs_autoload_enabled));
}

/*****************************************************************************/
// MI_INFO_HEATBREAK_N_TEMP
#if ENABLED(PRUSA_TOOLCHANGER)
I_MI_INFO_HEATBREAK_N_TEMP::I_MI_INFO_HEATBREAK_N_TEMP(const char *const specific_label, int index)
    : WI_TEMP_LABEL_t(prusa_toolchanger.is_toolchanger_enabled() ? _(specific_label) : _(generic_label), //< Toolchanger has specific labels
        nullptr, is_enabled_t::yes,
        ((index == 0) || (prusa_toolchanger.is_toolchanger_enabled() && dwarfs[index].is_enabled())) ? is_hidden_t::no : is_hidden_t::yes) { //< Index 0 is never hidden
}
#else  /*ENABLED(PRUSA_TOOLCHANGER)*/
I_MI_INFO_HEATBREAK_N_TEMP::I_MI_INFO_HEATBREAK_N_TEMP(const char *const specific_label, int index)
    : WI_TEMP_LABEL_t(_(generic_label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/
/*****************************************************************************/
//MI_PRINT_PROGRESS_TIME
MI_PRINT_PROGRESS_TIME::MI_PRINT_PROGRESS_TIME()
    : WiSpinInt(variant8_get_ui16(eeprom_get_var(EEVAR_PRINT_PROGRESS_TIME)),
        SpinCnf::print_progress, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_PRINT_PROGRESS_TIME::OnClick() {
    eeprom_set_var(EEVAR_PRINT_PROGRESS_TIME, variant8_ui16(GetVal()));
}

/*****************************************************************************/
// MI_INFO_BED_TEMP
MI_INFO_BED_TEMP::MI_INFO_BED_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_INFO_NOZZLE_N_TEMP
#if ENABLED(PRUSA_TOOLCHANGER)
I_MI_INFO_NOZZLE_N_TEMP::I_MI_INFO_NOZZLE_N_TEMP(const char *const specific_label, int index)
    : WI_TEMP_LABEL_t(prusa_toolchanger.is_toolchanger_enabled() ? _(specific_label) : _(generic_label), //< Toolchanger has specific labels
        nullptr, is_enabled_t::yes,
        ((index == 0) || (prusa_toolchanger.is_toolchanger_enabled() && dwarfs[index].is_enabled())) ? is_hidden_t::no : is_hidden_t::yes) { //< Index 0 is never hidden
}
#else  /*ENABLED(PRUSA_TOOLCHANGER)*/
I_MI_INFO_NOZZLE_N_TEMP::I_MI_INFO_NOZZLE_N_TEMP(const char *const specific_label, int index)
    : WI_TEMP_LABEL_t(_(generic_label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/

/*****************************************************************************/
// MI_INFO_LOADCELL
MI_INFO_LOADCELL::MI_INFO_LOADCELL()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f", (double)value.float_val);
            } else {
                if (value.attribute.enabled) {
                    strlcpy(buffer, NA, GuiDefaults::infoDefaultLen);
                } else {
                    strlcpy(buffer, NI, GuiDefaults::infoDefaultLen);
                }
            }
        }) {
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
    : MI_ODOMETER_DIST(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, -1) {
}
MI_ODOMETER_TIME::MI_ODOMETER_TIME()
    : WI_FORMATABLE_LABEL_t<uint32_t>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, 0, [&](char *buffer) {
        time_t time = (time_t)value;
        constexpr static uint32_t secPerDay = 24 * 60 * 60;
        const struct tm *timeinfo = localtime(&time);
        if (timeinfo->tm_yday) {
            //days are recalculated, because timeinfo shows number of days in year and we want more days than 365
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

/*****************************************************************************/
// MI_INFO_NOZZLE_TEMP
MI_INFO_BOARD_TEMP::MI_INFO_BOARD_TEMP()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_FOOTER_RESET::MI_FOOTER_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FOOTER_RESET::click(IWindowMenu &window_menu) {
    // simple reset of footer eeprom would be better
    // but footer does not have reload method
    FooterItemHeater::ResetDrawMode();
    FooterLine::SetCenterN(footer::DefaultCenterNAndFewer);

    footer::eeprom::Store(footer::DefaultItems);
    //send event for all footers
    Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::EncodeItemForEvent(footer::items::count_));
}

/*****************************************************************************/
// MI_INFO_DWARF_MCU_TEMPERATURE
/*****************************************************************************/
#if BOARD_IS_XLBUDDY
MI_INFO_DWARF_BOARD_TEMPERATURE::MI_INFO_DWARF_BOARD_TEMPERATURE()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
#endif

#if ENABLED(MODULAR_HEATBED)
/**********************************************************************************************/
//MI_HEAT_ENTIRE_BED
bool MI_HEAT_ENTIRE_BED::init_index() const {
    return eeprom_get_bool(EEVAR_HEAT_ENTIRE_BED);
}

void MI_HEAT_ENTIRE_BED::OnChange(size_t old_index) {
    eeprom_set_bool(EEVAR_HEAT_ENTIRE_BED, !old_index);
    index = !old_index;
    if (index == 1) {
        marlin_gcode("M556 A"); // enable all bedlets now
    }
}

MI_INFO_MODULAR_BED_BOARD_TEMPERATURE::MI_INFO_MODULAR_BED_BOARD_TEMPERATURE()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
#endif
MI_HEATUP_BED::MI_HEATUP_BED()
    : WI_ICON_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_HEATUP_BED), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_HEATUP_BED::OnChange(size_t old_index) {
    eeprom_set_bool(EEVAR_HEATUP_BED, !old_index);
}
MI_INFO_SERIAL_NUM_LOVEBOARD::MI_INFO_SERIAL_NUM_LOVEBOARD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
MI_INFO_SERIAL_NUM_XLCD::MI_INFO_SERIAL_NUM_XLCD()
    : WiInfo<28>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
