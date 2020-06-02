#include "MItem_tools.hpp"
#include "dump.h"
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "wizard/wizard.h"
#include "marlin_client.h"
#include "gui.h"
#include "sys.h"
#include "window_dlg_wait.h"

/*****************************************************************************/
//MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_WIZARD::click(Iwindow_menu_t &window_menu) {
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

void MI_AUTO_HOME::click(Iwindow_menu_t &window_menu) {
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

void MI_MESH_BED::click(Iwindow_menu_t &window_menu) {
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

void MI_SELFTEST::click(Iwindow_menu_t &window_menu) {
    wizard_run_selftest();
}

/*****************************************************************************/
//MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_CALIB_FIRST::click(Iwindow_menu_t &window_menu) {
    wizard_run_firstlay();
}

/*****************************************************************************/
//MI_DISABLE_STEP
MI_DISABLE_STEP::MI_DISABLE_STEP()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_DISABLE_STEP::click(Iwindow_menu_t &window_menu) {
    marlin_gcode("M18");
}

/*****************************************************************************/
//MI_FACTORY_DEFAULTS
MI_FACTORY_DEFAULTS::MI_FACTORY_DEFAULTS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FACTORY_DEFAULTS::click(Iwindow_menu_t &window_menu) {
    if (gui_msgbox("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?", MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1) == MSGBOX_RES_YES) {
        eeprom_defaults();
        gui_msgbox("Factory defaults loaded. The system will now restart.", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
        sys_reset();
    }
}

/*****************************************************************************/
//MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SAVE_DUMP::click(Iwindow_menu_t &window_menu) {
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

void MI_HF_TEST_0::click(Iwindow_menu_t &window_menu) {
    dump_hardfault_test_0();
}

/*****************************************************************************/
//MI_HF_TEST_1
MI_HF_TEST_1::MI_HF_TEST_1()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_HF_TEST_1::click(Iwindow_menu_t &window_menu) {
    dump_hardfault_test_1();
}

/*****************************************************************************/
//MI_EE_LOAD_400
MI_EE_LOAD_400::MI_EE_LOAD_400()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_400::click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.0-final+1965.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_401
MI_EE_LOAD_401::MI_EE_LOAD_401()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_401::click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.1-final+1974.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_402
MI_EE_LOAD_402::MI_EE_LOAD_402()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_402::click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.2-final+1977.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403RC1
MI_EE_LOAD_403RC1::MI_EE_LOAD_403RC1()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_403RC1::click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-RC1+246.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD_403
MI_EE_LOAD_403::MI_EE_LOAD_403()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD_403::click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom/eeprom_MINI-4.0.3-final+258.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_LOAD
MI_EE_LOAD::MI_EE_LOAD()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_LOAD::click(Iwindow_menu_t &window_menu) {
    eeprom_load_bin_from_usb("eeprom.bin");
    sys_reset();
}

/*****************************************************************************/
//MI_EE_SAVE
MI_EE_SAVE::MI_EE_SAVE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_SAVE::click(Iwindow_menu_t &window_menu) {
    eeprom_save_bin_to_usb("eeprom.bin");
}

/*****************************************************************************/
//MI_EE_SAVEXML
MI_EE_SAVEXML::MI_EE_SAVEXML()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_EE_SAVEXML::click(Iwindow_menu_t &window_menu) {
    eeprom_save_xml_to_usb("eeprom.xml");
}
