#include "MItem_tools.hpp"
#include "dump.h"
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "wizard/wizard.h"
#include "marlin_client.h"
#include "gui.hpp"
#include "sys.h"
#include "window_dlg_wait.hpp"
#include "window_dlg_calib_z.hpp"
#include "sound.hpp"
#include "wui_api.h"
#include "../lang/i18n.h"

/*****************************************************************************/
//MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_WIZARD::click(IWindowMenu & /*window_menu*/) {
    wizard_run_complete();
}

/*****************************************************************************/
//workaroudn for MI_AUTO_HOME and MI_MESH_BED todo remove
int8_t gui_marlin_G28_or_G29_in_progress() {
    uint32_t cmd = marlin_command();
    if ((cmd == MARLIN_CMD_G28) || (cmd == MARLIN_CMD_G29))
        return -1;
    else
        return 0;
}

/*****************************************************************************/
//MI_AUTO_HOME
MI_AUTO_HOME::MI_AUTO_HOME()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_AUTO_HOME::click(IWindowMenu & /*window_menu*/) {
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    marlin_gcode("G28");
    while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
        marlin_client_loop();
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress, DLG_W8_DRAW_FRAME | DLG_W8_DRAW_HOURGLASS);
}

/*****************************************************************************/
//MI_MESH_BED
MI_MESH_BED::MI_MESH_BED()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_MESH_BED::click(IWindowMenu & /*window_menu*/) {
    if (!marlin_all_axes_homed()) {
        marlin_event_clr(MARLIN_EVT_CommandBegin);
        marlin_gcode("G28");
        while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
            marlin_client_loop();
        gui_dlg_wait(gui_marlin_G28_or_G29_in_progress, DLG_W8_DRAW_FRAME | DLG_W8_DRAW_HOURGLASS);
    }
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    marlin_gcode("G29");
    while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
        marlin_client_loop();
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress, DLG_W8_DRAW_FRAME | DLG_W8_DRAW_HOURGLASS);
}

/*****************************************************************************/
//MI_SELFTEST
MI_SELFTEST::MI_SELFTEST()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SELFTEST::click(IWindowMenu & /*window_menu*/) {
    wizard_run_selftest();
}

/*****************************************************************************/
//MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_CALIB_FIRST::click(IWindowMenu & /*window_menu*/) {
    wizard_run_firstlay();
}

/*****************************************************************************/
//MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_DISABLE_STEP::click(IWindowMenu & /*window_menu*/) {
    marlin_gcode("M18");
}

/*****************************************************************************/
//MI_FACTORY_DEFAULTS
MI_FACTORY_DEFAULTS::MI_FACTORY_DEFAULTS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FACTORY_DEFAULTS::click(IWindowMenu & /*window_menu*/) {
    if (gui_msgbox(_("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?"), MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1) == MSGBOX_RES_YES) {
        eeprom_defaults();
        gui_msgbox(_("Factory defaults loaded. The system will now restart."), MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
        sys_reset();
    }
}

/*****************************************************************************/
//MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SAVE_DUMP::click(IWindowMenu & /*window_menu*/) {
    if (dump_save_to_usb("dump.bin"))
        gui_msgbox(_("A crash dump report (file dump.bin) has been saved to the USB drive."), MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
    else
        gui_msgbox(_("Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again."), MSGBOX_BTN_OK | MSGBOX_ICO_ERROR);
}

/*****************************************************************************/
//MI_HF_TEST_0
MI_HF_TEST_0::MI_HF_TEST_0()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_HF_TEST_0::click(IWindowMenu & /*window_menu*/) {
    dump_hardfault_test_0();
}

/*****************************************************************************/
//MI_HF_TEST_1
MI_HF_TEST_1::MI_HF_TEST_1()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_HF_TEST_1::click(IWindowMenu & /*window_menu*/) {
    dump_hardfault_test_1();
}

/*****************************************************************************/
//MI_EE_LOAD_400
MI_EE_LOAD_400::MI_EE_LOAD_400()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_400::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.0-final+1965.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_401
MI_EE_LOAD_401::MI_EE_LOAD_401()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_401::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.1-final+1974.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_402
MI_EE_LOAD_402::MI_EE_LOAD_402()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_402::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.2-final+1977.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403RC1
MI_EE_LOAD_403RC1::MI_EE_LOAD_403RC1()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_403RC1::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-RC1+246.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403
MI_EE_LOAD_403::MI_EE_LOAD_403()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_403::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-final+258.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD
MI_EE_LOAD::MI_EE_LOAD()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD::click(IWindowMenu & /*window_menu*/) {
    eeprom_load_bin_from_usb("eeprom.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_SAVE
MI_EE_SAVE::MI_EE_SAVE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_SAVE::click(IWindowMenu & /*window_menu*/) {
    eeprom_save_bin_to_usb("eeprom.bin");
}

/*****************************************************************************/
//MI_EE_SAVEXML
MI_EE_SAVEXML::MI_EE_SAVEXML()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_SAVEXML::click(IWindowMenu & /*window_menu*/) {
    eeprom_save_xml_to_usb("eeprom.xml");
}

/*****************************************************************************/
//MI_M600
MI_M600::MI_M600()
    : WI_LABEL_t(label, 0, true, false) {
}
void MI_M600::click(IWindowMenu & /*window_menu*/) {
    marlin_gcode_push_front("M600");
}

/*****************************************************************************/
//MI_TIMEOUT
//if needed to remeber after poweroff
//use st25dv64k_user_read(MENU_TIMEOUT_FLAG_ADDRESS) st25dv64k_user_write((uint16_t)MENU_TIMEOUT_FLAG_ADDRESS, (uint8_t)1 or 0);
//todo do not use externed variables like menu_timeout_enabled
MI_TIMEOUT::MI_TIMEOUT()
    : WI_SWITCH_OFF_ON_t(menu_timeout_enabled ? 1 : 0, label, 0, true, false) {}
void MI_TIMEOUT::OnChange(size_t old_index) {
    if (old_index) {
        gui_timer_delete(gui_get_menu_timeout_id());
    }
    menu_timeout_enabled = !old_index;
}

/*****************************************************************************/
//MI_SOUND_MODE
size_t MI_SOUND_MODE::init_index() const {
    size_t sound_mode = Sound_GetMode();
    return sound_mode > 4 ? eSOUND_MODE_DEFAULT : sound_mode;
}
MI_SOUND_MODE::MI_SOUND_MODE()
    : WI_SWITCH_t<4>(init_index(), label, 0, true, false, str_Once, str_Loud, str_Silent, str_Assist) {}
void MI_SOUND_MODE::OnChange(size_t /*old_index*/) {
    Sound_SetMode(static_cast<eSOUND_MODE>(index));
}

/*****************************************************************************/
//MI_SOUND_TYPE
MI_SOUND_TYPE::MI_SOUND_TYPE()
    : WI_SWITCH_t<8>(0, label, 0, true, false, str_ButtonEcho, str_StandardPrompt, str_StandardAlert, str_CriticalAlert, str_EncoderMove, str_BlindAlert, str_Start, str_SingleBeep) {}
void MI_SOUND_TYPE::OnChange(size_t old_index) {
    if (old_index == eSOUND_TYPE_StandardPrompt || old_index == eSOUND_TYPE_CriticalAlert) {
        gui_msgbox_prompt(_("Continual beeps test\n press button to stop"), MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
    } else {
        Sound_Play(static_cast<eSOUND_TYPE>(old_index));
    }
}

/*****************************************************************************/
//MI_SOUND_VOLUME
constexpr static const std::array<uint8_t, 3> volume_range = { { 0, 10, 1 } };
MI_SOUND_VOLUME::MI_SOUND_VOLUME()
    : WI_SPIN_U08_t(static_cast<uint8_t>(Sound_GetVolume()), volume_range.data(), label, 0, true, false) {}
/* void MI_SOUND_VOLUME::Change(int dif) { */
/* int v = value - dif; */
/* Sound_SetVolume(value); */
/* } */
void MI_SOUND_VOLUME::OnClick() {
    Sound_SetVolume(value);
}

/*****************************************************************************/
//MI_TIMEZONE
constexpr static const std::array<int8_t, 3> timezone_range = { { -12, 12, 1 } };
MI_TIMEZONE::MI_TIMEZONE()
    : WI_SPIN_I08_t(eeprom_get_var(EEVAR_TIMEZONE).i8, timezone_range.data(), label, 0, true, false) {}
void MI_TIMEZONE::OnClick() {
    int8_t timezone = value;
    int8_t last_timezone = eeprom_get_var(EEVAR_TIMEZONE).i8;
    eeprom_set_var(EEVAR_TIMEZONE, variant8_i8(timezone));
    time_t seconds = 0;
    if ((seconds = sntp_get_system_time())) {
        sntp_set_system_time(seconds, last_timezone);
    }
}
