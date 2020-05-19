#include "WindowMenuItems.hpp"
#include "resource.h"
#include "cmath_ext.h"
#include "screen.h" //screen_close
#include "screens.h"

#define WIO_MIN  0 //todo remove
#define WIO_MAX  1
#define WIO_STEP 2

/*****************************************************************************/
//ctors
WI_LABEL_t::WI_LABEL_t(const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden) {}

WI_SPIN_t::WI_SPIN_t(int32_t value, const int32_t *range, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden)
    , value(value)
    , range(range) {}

WI_SPIN_FL_t::WI_SPIN_FL_t(float value, const float *range, const char *prt_format, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden)
    , value(value)
    , range(range)
    , prt_format(prt_format) {}

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

bool WI_LABEL_t::Change(int dif) {
    return false;
}

bool WI_SPIN_t::Change(int dif) {
    int32_t old = value;

    if (dif > 0) {
        value = MIN(value + dif * range[WIO_STEP], range[WIO_MAX]);
    } else {
        value = MAX(value + dif * range[WIO_STEP], range[WIO_MIN]);
    }

    return old != value;
}

bool WI_SPIN_FL_t::Change(int dif) {
    float old = value;

    if (dif > 0) {
        value = MIN(value + (float)dif * range[WIO_STEP], range[WIO_MAX]);
    } else {
        value = MAX(value + (float)dif * range[WIO_STEP], range[WIO_MIN]);
    }

    return old != value;
}

bool WI_SWITCH_t::Change(int dif) {
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

    if (dif > 0) {
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
