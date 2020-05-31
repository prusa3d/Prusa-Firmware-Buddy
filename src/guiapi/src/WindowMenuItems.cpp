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
