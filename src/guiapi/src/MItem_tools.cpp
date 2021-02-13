#include "MItem_tools.hpp"
#include "dump.h"
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
#include "screen_wizard.hpp"
#include "bsod.h"
#include "liveadjust_z.hpp"
#include "DialogHandler.hpp"
#include "selftest_MINI.h"
#include "filament_sensor.hpp"
#include "main.h"
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "menu_spin_config.hpp"
#include "DialogSelftestResult.hpp"

/*****************************************************************************/
//MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIZARD::click(IWindowMenu & /*window_menu*/) {
    ScreenWizard::Run(wizard_run_type_t::all);
}

/*****************************************************************************/
//MI_LIVE_ADJUST_Z
MI_LIVE_ADJUST_Z::MI_LIVE_ADJUST_Z()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_LIVE_ADJUST_Z::click(IWindowMenu & /*window_menu*/) {
    LiveAdjustZ::Show();
}

/*****************************************************************************/
//MI_AUTO_HOME
MI_AUTO_HOME::MI_AUTO_HOME()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
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
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MESH_BED::click(IWindowMenu & /*window_menu*/) {
    if (!marlin_all_axes_homed()) {
        marlin_event_clr(MARLIN_EVT_CommandBegin);
        marlin_gcode("G28");
        while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
            marlin_client_loop();
        gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
    }
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    marlin_gcode("G29");
    while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
        marlin_client_loop();
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
}

/*****************************************************************************/
//MI_SELFTEST
MI_SELFTEST::MI_SELFTEST()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SELFTEST::click(IWindowMenu & /*window_menu*/) {
    ScreenWizard::Run(wizard_run_type_t::selftest);
}

/*****************************************************************************/
//MI_SELFTEST_RESULT
MI_SELFTEST_RESULT::MI_SELFTEST_RESULT()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SELFTEST_RESULT::click(IWindowMenu & /*window_menu*/) {
    SelftestResultEEprom_t result;
    result.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));
    DialogSelftestResult::Show(result);
}

/*****************************************************************************/
//MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_CALIB_FIRST::click(IWindowMenu & /*window_menu*/) {
    ScreenWizard::Run(wizard_run_type_t::firstlay);
}

/*****************************************************************************/
//MI_TEST_FANS
MI_TEST_FANS::MI_TEST_FANS()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_FANS::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmFans);
    DialogHandler::WaitUntilClosed(ClientFSM::SelftestFans, 0);
}

/*****************************************************************************/
//MI_TEST_XYZ
MI_TEST_XYZ::MI_TEST_XYZ()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_XYZ::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmXYZAxis);
    DialogHandler::WaitUntilClosed(ClientFSM::SelftestAxis, 0);
}

/*****************************************************************************/
//MI_TEST_HEAT
MI_TEST_HEAT::MI_TEST_HEAT()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_HEAT::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmHeaters);
    DialogHandler::WaitUntilClosed(ClientFSM::SelftestHeat, 0);
}

/*****************************************************************************/
//MI_TEST_FANS_fine
MI_ADVANCED_FAN_TEST::MI_ADVANCED_FAN_TEST()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ADVANCED_FAN_TEST::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmFans_fine);
    DialogHandler::WaitUntilClosed(ClientFSM::SelftestFans, 0);
}

/*****************************************************************************/
//MI_TEST_ABORT
MI_TEST_ABORT::MI_TEST_ABORT()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_ABORT::click(IWindowMenu & /*window_menu*/) {
    marlin_test_abort();
}

/*****************************************************************************/
//MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_DISABLE_STEP::click(IWindowMenu & /*window_menu*/) {
    marlin_gcode("M18");
}

/*****************************************************************************/
//MI_FACTORY_DEFAULTS
MI_FACTORY_DEFAULTS::MI_FACTORY_DEFAULTS()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FACTORY_DEFAULTS::click(IWindowMenu & /*window_menu*/) {
    if (MsgBoxWarning(_("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?"), Responses_YesNo, 1) == Response::Yes) {
        eeprom_defaults();
        MsgBoxInfo(_("Factory defaults loaded. The system will now restart."), Responses_Ok);
        sys_reset();
    }
}

/*****************************************************************************/
//MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SAVE_DUMP::click(IWindowMenu & /*window_menu*/) {
    if (dump_save_to_usb("dump.bin"))
        MsgBoxInfo(_("A crash dump report (file dump.bin) has been saved to the USB drive."), Responses_Ok);
    else
        MsgBoxError(_("Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again."), Responses_Ok);
}

/*****************************************************************************/
//MI_XFLASH_DELETE
MI_XFLASH_DELETE::MI_XFLASH_DELETE()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_XFLASH_DELETE::click(IWindowMenu & /*window_menu*/) {
    dump_in_xflash_delete();
}

/*****************************************************************************/
//MI_XFLASH_RESET
MI_XFLASH_RESET::MI_XFLASH_RESET()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_XFLASH_RESET::click(IWindowMenu & /*window_menu*/) {
    dump_in_xflash_reset();
}

/*****************************************************************************/
//MI_HF_TEST_0
MI_HF_TEST_0::MI_HF_TEST_0()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_HF_TEST_0::click(IWindowMenu & /*window_menu*/) {
    dump_hardfault_test_0();
}

/*****************************************************************************/
//MI_HF_TEST_1
MI_HF_TEST_1::MI_HF_TEST_1()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_HF_TEST_1::click(IWindowMenu & /*window_menu*/) {
    dump_hardfault_test_1();
}

/*****************************************************************************/
//MI_EE_LOAD_400
MI_EE_LOAD_400::MI_EE_LOAD_400()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_400::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.0-final+1965.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_401
MI_EE_LOAD_401::MI_EE_LOAD_401()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_401::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.1-final+1974.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_402
MI_EE_LOAD_402::MI_EE_LOAD_402()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_402::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.2-final+1977.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403RC1
MI_EE_LOAD_403RC1::MI_EE_LOAD_403RC1()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_403RC1::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-RC1+246.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403
MI_EE_LOAD_403::MI_EE_LOAD_403()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD_403::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-final+258.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD
MI_EE_LOAD::MI_EE_LOAD()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_LOAD::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_SAVE
MI_EE_SAVE::MI_EE_SAVE()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_SAVE::click(IWindowMenu & /*window_menu*/) {
    eeprom_save_bin_to_usb("eeprom.bin");
}

/*****************************************************************************/
//MI_EE_SAVEXML
MI_EE_SAVEXML::MI_EE_SAVEXML()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EE_SAVEXML::click(IWindowMenu & /*window_menu*/) {
    eeprom_save_xml_to_usb("eeprom.xml");
}

/*****************************************************************************/
//MI_M600
MI_M600::MI_M600()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_M600::click(IWindowMenu & /*window_menu*/) {
    marlin_gcode_push_front("M600");
}

/*****************************************************************************/
//MI_TIMEOUT
MI_TIMEOUT::MI_TIMEOUT()
    : WI_SWITCH_OFF_ON_t(Screens::Access()->GetMenuTimeout() ? 1 : 0, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEOUT::OnChange(size_t old_index) {
    if (!old_index) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }
    eeprom_set_var(EEVAR_MENU_TIMEOUT, variant8_ui8((uint8_t)(Screens::Access()->GetMenuTimeout() ? 1 : 0)));
}

/*****************************************************************************/
//MI_SOUND_MODE
size_t MI_SOUND_MODE::init_index() const {
    eSOUND_MODE sound_mode = Sound_GetMode();
    return (size_t)(sound_mode > eSOUND_MODE::ASSIST ? eSOUND_MODE::DEFAULT : sound_mode);
}
MI_SOUND_MODE::MI_SOUND_MODE()
    : WI_SWITCH_t<MI_SOUND_MODE_COUNT>(init_index(), _(label), 0, is_enabled_t::yes, is_hidden_t::no,
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
    : WI_SWITCH_t<8>(0, _(label), 0, is_enabled_t::yes, is_hidden_t::no,
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
    : WI_SPIN_U08_t(static_cast<uint8_t>(Sound_GetVolume()), SpinCnf::volume_range, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_SOUND_VOLUME::OnClick() {
    Sound_SetVolume(GetVal());
}

/*****************************************************************************/
//MI_SORT_FILES
MI_SORT_FILES::MI_SORT_FILES()
    : WI_SWITCH_t<2>(variant_get_ui8(eeprom_get_var(EEVAR_FILE_SORT)), _(label), 0, is_enabled_t::yes, is_hidden_t::no, _(str_time), _(str_name)) {}
void MI_SORT_FILES::OnChange(size_t old_index) {
    if (old_index == WF_SORT_BY_TIME) { // default option - was sorted by time of change, set by name
        eeprom_set_var(EEVAR_FILE_SORT, variant8_ui8((uint8_t)WF_SORT_BY_NAME));
        screen_filebrowser_sort = WF_SORT_BY_NAME;
    } else if (old_index == WF_SORT_BY_NAME) { // was sorted by name, set by time
        eeprom_set_var(EEVAR_FILE_SORT, variant8_ui8((uint8_t)WF_SORT_BY_TIME));
        screen_filebrowser_sort = WF_SORT_BY_TIME;
    }
}

/*****************************************************************************/
//MI_TIMEZONE
MI_TIMEZONE::MI_TIMEZONE()
    : WI_SPIN_I08_t(variant8_get_i8(eeprom_get_var(EEVAR_TIMEZONE)), SpinCnf::timezone_range, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEZONE::OnClick() {
    int8_t timezone = GetVal();
    int8_t last_timezone = variant8_get_i8(eeprom_get_var(EEVAR_TIMEZONE));
    eeprom_set_var(EEVAR_TIMEZONE, variant8_i8(timezone));
    time_t seconds = 0;
    if ((seconds = sntp_get_system_time())) {
        sntp_set_system_time(seconds, last_timezone);
    }
}

/*****************************************************************************/
//I_MI_Filament
void I_MI_Filament::click_at(filament_t filament_index) {
    const Filament filament = Filaments::Get(filament_index);
    /// don't use preheat temp for cooldown
    if (Filaments::PreheatTemp >= filament.nozzle) {
        marlin_gcode_printf("M104 S%d", (int)filament.nozzle);
    } else {
        marlin_gcode_printf("M104 S%d D%d", (int)Filaments::PreheatTemp, (int)filament.nozzle);
    }
    marlin_gcode_printf("M140 S%d", (int)filament.heatbed);
    Filaments::SetLastPreheated(filament_index);
    Screens::Access()->Close(); // skip this screen everytime
}

MI_FILAMENT_SENSOR_STATE::MI_FILAMENT_SENSOR_STATE()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), 0, is_enabled_t::no, is_hidden_t::no) {
}

MI_FILAMENT_SENSOR_STATE::state_t MI_FILAMENT_SENSOR_STATE::get_state() {
    fsensor_t fs = FS_instance().WaitInitialized();
    switch (fs) {
    case fsensor_t::HasFilament:
        return state_t::high;
    case fsensor_t::NoFilament:
        return state_t::low;
    default:;
    }
    return state_t::unknown;
}

bool MI_FILAMENT_SENSOR_STATE::StateChanged() {
    return SetIndex((size_t)get_state());
}

MI_MINDA::MI_MINDA()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), 0, is_enabled_t::no, is_hidden_t::no) {
}

MI_MINDA::state_t MI_MINDA::get_state() {
    return (buddy::hw::zMin.read() == buddy::hw::Pin::State::low) ? state_t::low : state_t::high;
}

bool MI_MINDA::StateChanged() {
    return SetIndex((size_t)get_state());
}

/*****************************************************************************/
//MI_FAN_CHECK
MI_FAN_CHECK::MI_FAN_CHECK()
    : WI_SWITCH_OFF_ON_t(variant_get_ui8(marlin_get_var(MARLIN_VAR_FAN_CHECK_ENABLED)), _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_FAN_CHECK::OnChange(size_t old_index) {
    if (!old_index) {
        marlin_set_var(MARLIN_VAR_FAN_CHECK_ENABLED, variant8_ui8(1));
    } else {
        marlin_set_var(MARLIN_VAR_FAN_CHECK_ENABLED, variant8_ui8(0));
    }
    eeprom_set_var(EEVAR_FAN_CHECK_ENABLED, variant8_ui8(marlin_get_var(MARLIN_VAR_FAN_CHECK_ENABLED)));
}

/*****************************************************************************/
//MI_FS_AUTOLOAD
is_hidden_t hide_autoload_item() {
    return FS_instance().Get() == fsensor_t::Disabled ? is_hidden_t::yes : is_hidden_t::no;
}

MI_FS_AUTOLOAD::MI_FS_AUTOLOAD()
    : WI_SWITCH_OFF_ON_t(variant_get_ui8(marlin_get_var(MARLIN_VAR_FS_AUTOLOAD_ENABLED)), _(label), 0, is_enabled_t::yes, hide_autoload_item()) {}
void MI_FS_AUTOLOAD::OnChange(size_t old_index) {
    if (!old_index) {
        marlin_set_var(MARLIN_VAR_FS_AUTOLOAD_ENABLED, variant8_ui8(1));
    } else {
        marlin_set_var(MARLIN_VAR_FS_AUTOLOAD_ENABLED, variant8_ui8(0));
    }
    eeprom_set_var(EEVAR_FS_AUTOLOAD_ENABLED, variant8_ui8(marlin_get_var(MARLIN_VAR_FS_AUTOLOAD_ENABLED)));
}
