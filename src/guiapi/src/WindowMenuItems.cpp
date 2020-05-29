#include "WindowMenuItems.hpp"
#include "resource.h"
#include "screen.h" //screen_close
#include "screens.h"

/*****************************************************************************/
//ctors
WI_LABEL_t::WI_LABEL_t(const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden) {}

WI_SWITCH_t::WI_SWITCH_t(int32_t index, const char **strings, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

WI_SELECT_t::WI_SELECT_t(int32_t index, const char **strings, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

/*****************************************************************************/
//return changed (== invalidate)

bool WI_LABEL_t::Change(int) {
    return false;
}

bool WI_SWITCH_t::Change(int) {
    size_t size = 0;
    while (strings[size] != NULL) {
        size++;
    }
    ++index;
    if (index >= size) {
        index = 0;
    }
    return true;
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
