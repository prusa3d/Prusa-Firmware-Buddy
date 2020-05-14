#include "WindowMenuItems.hpp"
#include "resource.h"
#include "cmath_ext.h"
#include "screen.h" //screen_close

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
const char *const MI_RETURN::label = "Return";

MI_RETURN::MI_RETURN()
    : WI_LABEL_t(label, IDR_PNG_filescreen_icon_up_folder, true, false) {
}
/*
int MI_RETURN::OnClick() {
    screen_close();
    return 1;
}*/
void MI_RETURN::Click() {
    screen_close();
}
