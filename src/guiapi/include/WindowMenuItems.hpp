#pragma once

#include "IWindowMenuItem.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"

#pragma pack(push, 1)
//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
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
    virtual void click(Iwindow_menu_t &window_menu) final;

public:
    WI_SPIN_t(T value, const T *range, const char *prt_format, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
    virtual void OnClick() {}
};

//WI_SWITCH == text version of WI_SPIN (non-numeric)
//unlike WI_SPIN cannot be selected
template <size_t SZ>
class WI_SWITCH_t : public IWindowMenuItem {
public: //todo private
    size_t index;
    const std::array<const char *, SZ> items;

protected:
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const;
    virtual void click(Iwindow_menu_t &window_menu) final;

public:
    //cannot create const std::array<const char *, SZ> with std::initializer_list<const char*>
    //template<class ...E> and {{std::forward<E>(e)...}} is workaround
    template <class... E>
    WI_SWITCH_t(int32_t index, const char *label, uint16_t id_icon, bool enabled, bool hidden, E &&... e)
        : IWindowMenuItem(label, id_icon, enabled, hidden)
        , index(index)
        , items { { std::forward<E>(e)... } } {}
    virtual bool Change(int dif);
    virtual void ClrIndex() { index = 0; }

protected:
    virtual void OnChange(size_t old_index) = 0;
};

//most common version of WI_SWITCH with on/off options
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = "Off";
    constexpr static const char *str_On = "On";

public:
    WI_SWITCH_OFF_ON_t(bool index, const char *const label, uint16_t id_icon, bool enabled, bool hidden)
        : WI_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, str_Off, str_On) {}
};

//WI_SELECT == switch with no label
//but can be selected like WI_SPIN
class WI_SELECT_t : public IWindowMenuItem {
    constexpr static const char *no_lbl = "";

public: //todo private
    uint32_t index;
    const char **strings;

protected:
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const;

public:
    WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
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
bool WI_SPIN_t<T>::Change(int dif) {
    T old = value;
    value += (T)dif * range[WIO_STEP];
    value = std::min(value, range[WIO_MAX]);
    value = std::max(value, range[WIO_MIN]);
    return old != value;
}

template <class T>
void WI_SPIN_t<T>::click(Iwindow_menu_t &window_menu) {
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
//template definitions
template <size_t SZ>
bool WI_SWITCH_t<SZ>::Change(int) {
    if ((++index) >= items.size()) {
        index = 0;
    }
    return true;
}

template <size_t SZ>
void WI_SWITCH_t<SZ>::click(Iwindow_menu_t &window_menu) {
    size_t old_index = index;
    Change(0);
    OnChange(old_index);
}

template <size_t SZ>
void WI_SWITCH_t<SZ>::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    IWindowMenuItem::printText(window_menu, rect, color_text, color_back, swap);
    const char *txt = items[index];

    rect_ui16_t vrc = {
        uint16_t(rect.x + rect.w), rect.y, uint16_t(window_menu.font->w * strlen(txt) + window_menu.padding.left + window_menu.padding.right), rect.h
    };
    vrc.x -= vrc.w;
    rect.w -= vrc.w;

    render_text_align(vrc, txt, window_menu.font,
        color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.alignment);
}

/*****************************************************************************/
//advanced types
class MI_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Return";

public:
    MI_RETURN();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
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

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_VERSION_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "Version Info";

public:
    MI_VERSION_INFO();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_SYS_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "System Info";

public:
    MI_SYS_INFO();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_STATISTIC_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Statistic";

public:
    MI_STATISTIC_disabled();

protected:
    virtual void click(Iwindow_menu_t &window_menu) {}
};

class MI_FAIL_STAT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Fail Stats";

public:
    MI_FAIL_STAT_disabled();

protected:
    virtual void click(Iwindow_menu_t &window_menu) {}
};

class MI_SUPPORT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Support";

public:
    MI_SUPPORT_disabled();

protected:
    virtual void click(Iwindow_menu_t &window_menu) {}
};

class MI_QR_test : public WI_LABEL_t {
    static constexpr const char *const label = "QR test";

public:
    MI_QR_test();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_QR_info : public WI_LABEL_t {
    static constexpr const char *const label = "Send Info by QR";

public:
    MI_QR_info();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_WIZARD : public WI_LABEL_t {
    static constexpr const char *const label = "Wizard";

public:
    MI_WIZARD();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_AUTO_HOME : public WI_LABEL_t {
    static constexpr const char *const label = "Auto Home";

public:
    MI_AUTO_HOME();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_MESH_BED : public WI_LABEL_t {
    static constexpr const char *const label = "Mesh Bed Level.";

public:
    MI_MESH_BED();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_SELFTEST : public WI_LABEL_t {
    static constexpr const char *const label = "SelfTest";

public:
    MI_SELFTEST();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_CALIB_FIRST : public WI_LABEL_t {
    static constexpr const char *const label = "First Layer Cal.";

public:
    MI_CALIB_FIRST();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_TEMPERATURE : public WI_LABEL_t {
    static constexpr const char *const label = "Temperature";

public:
    MI_TEMPERATURE();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_MOVE_AXIS : public WI_LABEL_t {
    static constexpr const char *const label = "Move Axis";

public:
    MI_MOVE_AXIS();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_DISABLE_STEP : public WI_LABEL_t {
    static constexpr const char *const label = "Disable Steppers";

public:
    MI_DISABLE_STEP();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_FACTORY_DEFAULTS : public WI_LABEL_t {
    static constexpr const char *const label = "Factory Reset";

public:
    MI_FACTORY_DEFAULTS();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_SERVICE : public WI_LABEL_t {
    static constexpr const char *const label = "Service";

public:
    MI_SERVICE();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_TEST : public WI_LABEL_t {
    static constexpr const char *const label = "Test";

public:
    MI_TEST();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_FW_UPDATE : public WI_LABEL_t {
    static constexpr const char *const label = "FW Update";

public:
    MI_FW_UPDATE();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_SAVE_DUMP : public WI_LABEL_t {
    static constexpr const char *const label = "Save Crash Dump";

public:
    MI_SAVE_DUMP();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_HF_TEST_0 : public WI_LABEL_t {
    static constexpr const char *const label = "HF0 test";

public:
    MI_HF_TEST_0();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_HF_TEST_1 : public WI_LABEL_t {
    static constexpr const char *const label = "HF1 test";

public:
    MI_HF_TEST_1();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_400 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.0";

public:
    MI_EE_LOAD_400();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_401 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.1";

public:
    MI_EE_LOAD_401();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_402 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.2";

public:
    MI_EE_LOAD_402();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_403RC1 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3-RC1";

public:
    MI_EE_LOAD_403RC1();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_403 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3";

public:
    MI_EE_LOAD_403();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD : public WI_LABEL_t {
    static constexpr const char *const label = "EE load";

public:
    MI_EE_LOAD();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_SAVE : public WI_LABEL_t {
    static constexpr const char *const label = "EE save";

public:
    MI_EE_SAVE();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_SAVEXML : public WI_LABEL_t {
    static constexpr const char *const label = "EE save xml";

public:
    MI_EE_SAVEXML();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

#pragma pack(pop)
