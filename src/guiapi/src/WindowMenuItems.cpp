#include "WindowMenuItems.hpp"
#include "resource.h"
#include "screen.h" //screen_close
#include "screens.h"
#include "wizard/wizard.h"
#include "marlin_client.h"
#include "window_dlg_wait.h"
#include "dump.h"
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "gui.h"
#include "sys.h"

/*****************************************************************************/
//ctors
WI_LABEL_t::WI_LABEL_t(const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden) {}

WI_SELECT_t::WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(no_lbl, id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

/*****************************************************************************/
//return changed (== invalidate)

bool WI_LABEL_t::Change(int) {
    return false;
}

bool WI_SELECT_t::Change(int dif) {
    size_t size = 0;
    while (strings[size] != NULL) {
        size++;
    }

    if (dif >= 0) {
        ++index;
        if (index >= size) {
            index = 0;
        }
    } else {
        --index;
        if (index < 0) {
            index = size - 1;
        }
    }

    return true;
}

/*****************************************************************************/

void WI_SELECT_t::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    IWindowMenuItem::printText(window_menu, rect, color_text, color_back, swap);
    const char *txt = strings[index];

    rect_ui16_t vrc = {
        uint16_t(rect.x + rect.w), rect.y, uint16_t(window_menu.font->w * strlen(txt) + window_menu.padding.left + window_menu.padding.right), rect.h
    };
    vrc.x -= vrc.w;
    rect.w -= vrc.w;

    render_text_align(vrc, txt, window_menu.font,
        color_back, color_text, window_menu.padding, window_menu.alignment);
}

/*****************************************************************************/
//specific WindowMenuItems

/*****************************************************************************/
//MI_RETURN
MI_RETURN::MI_RETURN()
    : WI_LABEL_t(label, IDR_PNG_filescreen_icon_up_folder, true, false) {
}

void MI_RETURN::Click(Iwindow_menu_t &window_menu) {
    screen_close();
}

/*****************************************************************************/
//MI_VERSION_INFO
MI_VERSION_INFO::MI_VERSION_INFO()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_VERSION_INFO::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_version_info()->id);
}

/*****************************************************************************/
//MI_SYS_INFO
MI_SYS_INFO::MI_SYS_INFO()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SYS_INFO::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_sysinfo()->id);
}

/*****************************************************************************/
//MI_STATISTIC_disabled
MI_STATISTIC_disabled::MI_STATISTIC_disabled()
    : WI_LABEL_t(label, 0, false, false) {
}

/*****************************************************************************/
//MI_FAIL_STAT_disabled
MI_FAIL_STAT_disabled::MI_FAIL_STAT_disabled()
    : WI_LABEL_t(label, 0, false, false) {
}

/*****************************************************************************/
//MI_SUPPORT_disabled
MI_SUPPORT_disabled::MI_SUPPORT_disabled()
    : WI_LABEL_t(label, 0, false, false) {
}

/*****************************************************************************/
//MI_QR_test
MI_QR_test::MI_QR_test()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_QR_test::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_qr_error()->id);
}

/*****************************************************************************/
//MI_QR_info
MI_QR_info::MI_QR_info()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_QR_info::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_qr_info()->id);
}

/*****************************************************************************/
//MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_WIZARD::Click(Iwindow_menu_t &window_menu) {
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

void MI_AUTO_HOME::Click(Iwindow_menu_t &window_menu) {
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    marlin_gcode("G28");
    while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
        marlin_client_loop();
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
}

/*****************************************************************************/
//MI_MESH_BED
MI_MESH_BED::MI_MESH_BED()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_MESH_BED::Click(Iwindow_menu_t &window_menu) {
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
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SELFTEST::Click(Iwindow_menu_t &window_menu) {
    wizard_run_selftest();
}

/*****************************************************************************/
//MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_CALIB_FIRST::Click(Iwindow_menu_t &window_menu) {
    wizard_run_firstlay();
}

/*****************************************************************************/
//MI_TEMPERATURE
MI_TEMPERATURE::MI_TEMPERATURE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_TEMPERATURE::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_menu_temperature()->id);
}

/*****************************************************************************/
//MI_MOVE_AXIS
MI_MOVE_AXIS::MI_MOVE_AXIS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_MOVE_AXIS::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_menu_move()->id);
}

/*****************************************************************************/
//MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_DISABLE_STEP::Click(Iwindow_menu_t &window_menu) {
    marlin_gcode("M18");
}

/*****************************************************************************/
//MI_FACTORY_DEFAULTS
MI_FACTORY_DEFAULTS::MI_FACTORY_DEFAULTS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FACTORY_DEFAULTS::Click(Iwindow_menu_t &window_menu) {
    if (gui_msgbox("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?", MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1) == MSGBOX_RES_YES) {
        eeprom_defaults();
        gui_msgbox("Factory defaults loaded. The system will now restart.", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
        sys_reset();
    }
}

/*****************************************************************************/
//MI_SERVICE
MI_SERVICE::MI_SERVICE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SERVICE::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_menu_service()->id);
}

/*****************************************************************************/
//MI_TEST
MI_TEST::MI_TEST()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_TEST::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_test()->id);
}

/*****************************************************************************/
//MI_FW_UPDATE
MI_FW_UPDATE::MI_FW_UPDATE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FW_UPDATE::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_menu_fw_update()->id);
}

/*****************************************************************************/
//MI_LAN_SETTINGS
MI_LAN_SETTINGS::MI_LAN_SETTINGS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_LAN_SETTINGS::Click(Iwindow_menu_t &window_menu) {
    screen_open(get_scr_lan_settings()->id);
}

/*****************************************************************************/
//MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SAVE_DUMP::Click(Iwindow_menu_t &window_menu) {
    if (dump_save_to_usb("dump.bin"))
        gui_msgbox("A crash dump report (file dump.bin) has been saved to the USB drive.", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
    else
        gui_msgbox("Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again.", MSGBOX_BTN_OK | MSGBOX_ICO_ERROR);
}

/*****************************************************************************/
//MI_HF_TEST_0
MI_HF_TEST_0::MI_HF_TEST_0()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_HF_TEST_0::Click(Iwindow_menu_t &window_menu) {
    dump_hardfault_test_0();
}

/*****************************************************************************/
//MI_HF_TEST_1
MI_HF_TEST_1::MI_HF_TEST_1()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_HF_TEST_1::Click(Iwindow_menu_t &window_menu) {
    dump_hardfault_test_1();
}

/*****************************************************************************/
//MI_EE_LOAD_400
MI_EE_LOAD_400::MI_EE_LOAD_400()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_400::Click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.0-final+1965.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_401
MI_EE_LOAD_401::MI_EE_LOAD_401()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_401::Click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.1-final+1974.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_402
MI_EE_LOAD_402::MI_EE_LOAD_402()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_402::Click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.2-final+1977.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403RC1
MI_EE_LOAD_403RC1::MI_EE_LOAD_403RC1()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_403RC1::Click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-RC1+246.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403
MI_EE_LOAD_403::MI_EE_LOAD_403()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_403::Click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-final+258.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD
MI_EE_LOAD::MI_EE_LOAD()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD::Click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_SAVE
MI_EE_SAVE::MI_EE_SAVE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_SAVE::Click(Iwindow_menu_t &window_menu) {
    eeprom_save_bin_to_usb("eeprom.bin");
}

/*****************************************************************************/
//MI_EE_SAVEXML
MI_EE_SAVEXML::MI_EE_SAVEXML()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_SAVEXML::Click(Iwindow_menu_t &window_menu) {
    eeprom_save_xml_to_usb("eeprom.xml");
}
