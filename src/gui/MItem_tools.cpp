#include "MItem_tools.hpp"
#include <crash_dump/dump.h>
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "marlin_client.h"
#include "gui.hpp"
#include "sys.h"
#include "window_dlg_wait.hpp"
#include "window_dlg_calib_z.hpp"
#include "window_file_list.hpp"
#include "sound.hpp"
#include "wui_api.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "filament_sensor_api.hpp"
#include "liveadjust_z.hpp"
#include "DialogHandler.hpp"
#include "filament_sensor.hpp"
#include "main.h"
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "menu_spin_config.hpp"
#include "footer_eeprom.hpp"
#include <time.h>
#include "sys.h"
#include "w25x.h"

/**********************************************************************************************/
//MI_FILAMENT_SENSOR
bool MI_FILAMENT_SENSOR::init_index() const {
    fsensor_t fs = FSensors_instance().GetPrinter();
    return fs == fsensor_t::Disabled ? 0 : 1;
}

void MI_FILAMENT_SENSOR::OnChange(size_t old_index) {
    if (old_index) {
        FSensors_instance().Disable();
    } else {
        FSensors_instance().Enable();

        // wait until it is processed
        // no guiloop here !!! - it could cause show of unwanted error message
        while (FSensors_instance().IsPrinter_processing_request()) {
            osDelay(0); // switch to other thread
        }

        fsensor_t state;
        // wait until it is initialized
        while ((state = FSensors_instance().GetPrinter()) == fsensor_t::NotInitialized) {
            osDelay(0); // switch to other thread
        }

        switch (state) {
        case fsensor_t::NotInitialized: //can't be we just checked it
            break;
        case fsensor_t::Disabled: // should not be
            MsgBoxError(_("Sensor logic error, printer filament sensor disabled."), Responses_Ok);
            index = old_index;
            break;
        case fsensor_t::NotCalibrated:
            MsgBoxWarning(_("Filament sensor not ready: perform calibration first. It is accessible from menu \"Calibrate\"."), Responses_Ok);
            index = old_index;
            FSensors_instance().Disable();
            break;
        case fsensor_t::HasFilament:
        case fsensor_t::NoFilament:
            break; // success
        case fsensor_t::NotConnected:
            MsgBoxError(_("Filament sensor not connected, check wiring."), Responses_Ok);
            index = old_index;
            FSensors_instance().Disable();
            break;
        }

        // wait until filament sensor command is processed (if any)
        // no guiloop here !!! - it could cause show of unwanted error message
        while (FSensors_instance().IsPrinter_processing_request()) {
            osDelay(0); // switch to other thread
        }
    }
}

/*****************************************************************************/
//MI_LIVE_ADJUST_Z
MI_LIVE_ADJUST_Z::MI_LIVE_ADJUST_Z()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
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
//MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_DISABLE_STEP::click(IWindowMenu & /*window_menu*/) {
    marlin_gcode("M18");
}

/*****************************************************************************/
//MI_FACTORY_DEFAULTS
MI_FACTORY_DEFAULTS::MI_FACTORY_DEFAULTS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_DEFAULTS::click(IWindowMenu & /*window_menu*/) {
    if (MsgBoxWarning(_("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?"), Responses_YesNo, 1) == Response::Yes) {
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

static inline void MsgBoxNonBlockInfo(string_view_utf8 txt) {
    constexpr static const char *title = N_("Information");
    MsgBoxTitled mbt(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, txt, is_multiline::yes, _(title), png::Get<png::Id::info_16x16>());
    gui::TickLoop();
    gui_loop();
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
    w25x_chip_erase();
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
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_HF_TEST_0::click(IWindowMenu & /*window_menu*/) {
    dump_hardfault_test_0();
}

/*****************************************************************************/
//MI_HF_TEST_1
MI_HF_TEST_1::MI_HF_TEST_1()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
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
    : WI_SWITCH_OFF_ON_t(Screens::Access()->GetMenuTimeout() ? 1 : 0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
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

MI_FILAMENT_SENSOR_STATE::MI_FILAMENT_SENSOR_STATE()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_FILAMENT_SENSOR_STATE::state_t MI_FILAMENT_SENSOR_STATE::get_state() {
    fsensor_t fs = FSensors_instance().GetPrinter();
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

/*****************************************************************************/
//MI_FAN_CHECK
MI_FAN_CHECK::MI_FAN_CHECK()
    : WI_SWITCH_OFF_ON_t(marlin_get_bool(MARLIN_VAR_FAN_CHECK_ENABLED), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_FAN_CHECK::OnChange(size_t old_index) {
    marlin_set_bool(MARLIN_VAR_FAN_CHECK_ENABLED, !old_index);
    eeprom_set_bool(EEVAR_FAN_CHECK_ENABLED, marlin_get_bool(MARLIN_VAR_FAN_CHECK_ENABLED));
}

/*****************************************************************************/
//MI_FS_AUTOLOAD
is_hidden_t hide_autoload_item() {
    return FSensors_instance().Get() == fsensor_t::Disabled ? is_hidden_t::yes : is_hidden_t::no;
}

MI_FS_AUTOLOAD::MI_FS_AUTOLOAD()
    : WI_SWITCH_OFF_ON_t(marlin_get_bool(MARLIN_VAR_FS_AUTOLOAD_ENABLED), _(label), nullptr, is_enabled_t::yes, hide_autoload_item()) {}
void MI_FS_AUTOLOAD::OnChange(size_t old_index) {
    marlin_set_bool(MARLIN_VAR_FS_AUTOLOAD_ENABLED, !old_index);
    eeprom_set_bool(EEVAR_FS_AUTOLOAD_ENABLED, marlin_get_bool(MARLIN_VAR_FS_AUTOLOAD_ENABLED));
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
    }) {
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
