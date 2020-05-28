#pragma once

#include "IWindowMenuItem.hpp"
#include <algorithm>
#include "display_helper.h"

#pragma pack(push, 1)
//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Incement(uint8_t dif);
    virtual bool Decrement(uint8_t dif);
};

//WI_SPIN
template <class T>
class WI_SPIN_t : public IWindowMenuItem {
    enum { WIO_MIN = 0,
        WIO_MAX = 1,
        WIO_STEP = 2 };

public: //todo private
    T value;
    const T *range;
    const char *prt_format;

protected:
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const;

public:
    WI_SPIN_t(T value, const T *range, const char *prt_format, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Incement(uint8_t dif);
    virtual bool Decrement(uint8_t dif);
    virtual void Click(Iwindow_menu_t &window_menu) final;
    virtual void OnClick() = 0;
};

//WI_SWITCH
//array of char strings ended by NULL for array length variability.
//char * strings[3] = {"Low", "High", "Medium", NULL}
class WI_SWITCH_t : public IWindowMenuItem {
public: //todo private
    uint32_t index;
    const char **strings;

public:
    WI_SWITCH_t(int32_t index, const char **strings, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Incement(uint8_t dif);
    virtual bool Decrement(uint8_t dif);
};

//WI_SELECT
//array of char strings ended by NULL for array length variability.
//char * strings[3] = {"Low", "High", "Medium", NULL}
class WI_SELECT_t : public IWindowMenuItem {
public: //todo private
    uint32_t index;
    const char **strings;

public:
    WI_SELECT_t(int32_t index, const char **strings, const char *label, uint16_t id_icon, bool enabled = true, bool hidden = false);
    virtual bool Incement(uint8_t dif);
    virtual bool Decrement(uint8_t dif);
};

/*****************************************************************************/
//template definitions
//WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T value, const T *range, const char *prt_format, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden)
    , value(value)
    , range(range)
    , prt_format(prt_format) {}

template <class T>
bool WI_SPIN_t<T>::Incement(uint8_t dif) {
    T old = value;
    value = std::min(value + (T)dif * range[WIO_STEP], range[WIO_MAX]);
    return old != value;
}

template <class T>
bool WI_SPIN_t<T>::Decrement(uint8_t dif) {
    T old = value;
    value = std::max(value - (T)dif * range[WIO_STEP], range[WIO_MIN]);
    return old != value;
}

template <class T>
void WI_SPIN_t<T>::Click(Iwindow_menu_t &window_menu) {
    if (selected) {
        OnClick();
    }
    selected = !selected;
}

template <class T>
void WI_SPIN_t<T>::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    IWindowMenuItem::printText(window_menu, rect, color_text, color_back, swap);
    char buff[20] = { '\0' };
    //value is template, it can be float in this case it would throw warning "-Wdouble-promotion"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
    snprintf(buff, 20, prt_format, value);
#pragma GCC diagnostic pop

    rect_ui16_t vrc = {
        uint16_t(rect.x + rect.w), rect.y, uint16_t(window_menu.font->w * strlen(buff) + window_menu.padding.left + window_menu.padding.right), rect.h
    };
    vrc.x -= vrc.w;
    rect.w -= vrc.w;

    render_text_align(vrc, buff, window_menu.font,
        color_back, IsSelected() ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.alignment);
}

/*****************************************************************************/
//advanced types
class MI_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Return";

public:
    MI_RETURN();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_FILAMENT : public WI_LABEL_t {
    static constexpr const char *const label = "Filament";

public:
    MI_FILAMENT();
};

class MI_LAN_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = "Lan settings";

public:
    MI_LAN_SETTINGS();
};

class MI_VERSION_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "Version Info";

public:
    MI_VERSION_INFO();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_SYS_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "System Info";

public:
    MI_SYS_INFO();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_STATISTIC_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Statistic";

public:
    MI_STATISTIC_disabled();
    virtual void Click(Iwindow_menu_t &window_menu) {}
};

class MI_FAIL_STAT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Fail Stats";

public:
    MI_FAIL_STAT_disabled();
    virtual void Click(Iwindow_menu_t &window_menu) {}
};

class MI_SUPPORT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Support";

public:
    MI_SUPPORT_disabled();
    virtual void Click(Iwindow_menu_t &window_menu) {}
};

class MI_QR_test : public WI_LABEL_t {
    static constexpr const char *const label = "QR test";

public:
    MI_QR_test();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_QR_info : public WI_LABEL_t {
    static constexpr const char *const label = "Send Info by QR";

public:
    MI_QR_info();
    virtual void Click(Iwindow_menu_t &window_menu);
};
#pragma pack(pop)
