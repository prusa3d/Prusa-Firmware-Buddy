#pragma once

#include "IWindowMenuItem.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "super.hpp"
#include "../lang/i18n.h"

//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif) override;
};

//IWiSpin
class IWiSpin : public IWindowMenuItem {
protected:
    enum { WIO_MIN = 0,
        WIO_MAX = 1,
        WIO_STEP = 2 };
    static std::array<char, 10> temp_buff; //temporary buffer to print value for text measurements
public:
    IWiSpin(const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : IWindowMenuItem(label, id_icon, enabled, hidden) {}
};

//WI_SPIN
template <class T>
class WI_SPIN_t : public AddSuper<IWiSpin> {

public: //todo private
    T value;
    const T *range;
    const char *prt_format;

protected:
    char *sn_prt() const;
    rect_ui16_t getSpinRect(Iwindow_menu_t &window_menu, rect_ui16_t base_rolling_rect, size_t spin_strlen) const;
    std::array<rect_ui16_t, 2> getRollingSpinRects(Iwindow_menu_t &window_menu, rect_ui16_t rect) const;
    virtual rect_ui16_t getRollingRect(Iwindow_menu_t &window_menu, rect_ui16_t rect) const override;
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const override;
    virtual void click(Iwindow_menu_t &window_menu) final;

public:
    WI_SPIN_t(T value, const T *range, const char *prt_format, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif) override;
    virtual void OnClick() {}
    void ClrVal() { value = static_cast<T>(0); }
};

//WI_SPIN_I08_t
//defines print format for int8_t version of WI_SPIN_t
class WI_SPIN_I08_t : public WI_SPIN_t<int8_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I08_t(int8_t value, const int8_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<int8_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_I16_t
//defines print format for int16_t version of WI_SPIN_t
class WI_SPIN_I16_t : public WI_SPIN_t<int16_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I16_t(int16_t value, const int16_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<int16_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_I32_t
//defines print format for int32_t version of WI_SPIN_t
class WI_SPIN_I32_t : public WI_SPIN_t<int32_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I32_t(int32_t value, const int32_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<int32_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U08_t
//defines print format for uint8_t version of WI_SPIN_t
class WI_SPIN_U08_t : public WI_SPIN_t<uint8_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U08_t(uint8_t value, const uint8_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<uint8_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U16_t
//defines print format for uint16_t version of WI_SPIN_t
class WI_SPIN_U16_t : public WI_SPIN_t<uint16_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U16_t(uint16_t value, const uint16_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<uint16_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U32_t
//defines print format for uint32_t version of WI_SPIN_t
class WI_SPIN_U32_t : public WI_SPIN_t<uint32_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U32_t(uint32_t value, const uint32_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<uint32_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SWITCH == text version of WI_SPIN (non-numeric)
//unlike WI_SPIN cannot be selected
//todo try to inherit from WI_SPIN<const char**> lot of code could be reused
template <size_t SZ>
class WI_SWITCH_t : public AddSuper<IWindowMenuItem> {
public: //todo private
    size_t index;
    const std::array<const char *, SZ> items;

protected:
    rect_ui16_t getSpinRect(Iwindow_menu_t &window_menu, rect_ui16_t base_rolling_rect, size_t spin_strlen) const;
    std::array<rect_ui16_t, 2> getRollingSpinRects(Iwindow_menu_t &window_menu, rect_ui16_t base_rolling_rect) const;
    virtual rect_ui16_t getRollingRect(Iwindow_menu_t &window_menu, rect_ui16_t rect) const override;
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const override;
    virtual void click(Iwindow_menu_t &window_menu) final;

public:
    //cannot create const std::array<const char *, SZ> with std::initializer_list<const char*>
    //template<class ...E> and {{std::forward<E>(e)...}} is workaround
    template <class... E>
    WI_SWITCH_t(int32_t index, const char *label, uint16_t id_icon, bool enabled, bool hidden, E &&... e)
        : AddSuper<IWindowMenuItem>(label, id_icon, enabled, hidden)
        , index(index)
        , items { { std::forward<E>(e)... } } {}
    virtual bool Change(int dif);
    virtual void ClrIndex() { index = 0; }
    virtual bool SetIndex(size_t idx);

protected:
    virtual void OnChange(size_t old_index) = 0;
};

//most common version of WI_SWITCH with on/off options
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = N_("Off");
    constexpr static const char *str_On = N_("On");

public:
    WI_SWITCH_OFF_ON_t(bool index, const char *const label, uint16_t id_icon, bool enabled, bool hidden)
        : WI_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, str_Off, str_On) {}
};

//currently broken todo FIXME
//WI_SELECT == switch with no label
//but can be selected like WI_SPIN
class WI_SELECT_t : public IWindowMenuItem {
    constexpr static const char *no_lbl = "";

public: //todo private
    uint32_t index;
    const char **strings;

protected:
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif) override;
};

/*****************************************************************************/
//template definitions
//WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T value, const T *range, const char *prt_format, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : AddSuper<IWiSpin>(label, id_icon, enabled, hidden)
    , value(value)
    , range(range)
    , prt_format(prt_format) {}

template <class T>
bool WI_SPIN_t<T>::Change(int dif) {
    T old = value;
    value += (T)dif * range[WIO_STEP];
    value = dif >= 0 ? std::max(value, old) : std::min(value, old); //check overflow/underflow
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

//helper method - to be used by getRollingRect
template <class T>
rect_ui16_t WI_SPIN_t<T>::getSpinRect(Iwindow_menu_t &window_menu, rect_ui16_t base_rolling_rect, size_t spin_strlen) const {
    rect_ui16_t spin_rect = {
        uint16_t(base_rolling_rect.x + base_rolling_rect.w), base_rolling_rect.y, uint16_t(window_menu.font->w * spin_strlen + window_menu.padding.left + window_menu.padding.right), base_rolling_rect.h
    };
    spin_rect.x -= spin_rect.w;
    return spin_rect;
}

/**
 * returns array<rect_ui16_t,2>
 * with values of
 * {rolling_rect, spin_rect}
 **/
template <class T>
std::array<rect_ui16_t, 2> WI_SPIN_t<T>::getRollingSpinRects(Iwindow_menu_t &window_menu, rect_ui16_t rect) const {
    rect_ui16_t base_rolling_rect = super::getRollingRect(window_menu, rect);
    char *buff = sn_prt();
    rect_ui16_t spin_rect = getSpinRect(window_menu, base_rolling_rect, strlen(buff));

    rect_ui16_t rolling_rect = base_rolling_rect;
    rolling_rect.w = spin_rect.x - rolling_rect.x;
    return std::array<rect_ui16_t, 2> { rolling_rect, spin_rect };
}

template <class T>
rect_ui16_t WI_SPIN_t<T>::getRollingRect(Iwindow_menu_t &window_menu, rect_ui16_t rect) const {
    return getRollingSpinRects(window_menu, rect)[0];
}

template <class T>
void WI_SPIN_t<T>::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    std::array<rect_ui16_t, 2> rects = getRollingSpinRects(window_menu, rect);

    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.alignment);
    //draw spin
    render_text_align(rects[1], temp_buff.data(), window_menu.font,
        color_back, IsSelected() ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.alignment);
}

template <class T>
char *WI_SPIN_t<T>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), prt_format, value);
    return temp_buff.data();
}

template <>
inline char *WI_SPIN_t<float>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), prt_format, static_cast<double>(value));
    return temp_buff.data();
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
bool WI_SWITCH_t<SZ>::SetIndex(size_t idx) {
    if (idx >= SZ)
        return false;
    else {
        index = idx;
        return true;
    }
}

//helper method - to be used by getRollingRect
template <size_t SZ>
rect_ui16_t WI_SWITCH_t<SZ>::getSpinRect(Iwindow_menu_t &window_menu, rect_ui16_t base_rolling_rect, size_t spin_strlen) const {
    rect_ui16_t spin_rect = base_rolling_rect;
    uint16_t spin_w = window_menu.font->w * spin_strlen + window_menu.padding.left + window_menu.padding.right;
    spin_rect.x = base_rolling_rect.x + base_rolling_rect.w - spin_w;
    spin_rect.w = spin_w;
    return spin_rect;
}

/**
 * returns array<rect_ui16_t,2>
 * with values of
 * {rolling_rect, spin_rect}
 **/
template <size_t SZ>
std::array<rect_ui16_t, 2> WI_SWITCH_t<SZ>::getRollingSpinRects(Iwindow_menu_t &window_menu, rect_ui16_t rect) const {
    rect_ui16_t base_rolling_rect = super::getRollingRect(window_menu, rect);
    rect_ui16_t spin_rect = getSpinRect(window_menu, base_rolling_rect, strlen(items[index]));

    rect_ui16_t rolling_rect = base_rolling_rect;
    rolling_rect.w = spin_rect.x - rolling_rect.x;
    return std::array<rect_ui16_t, 2> { rolling_rect, spin_rect };
}

template <size_t SZ>
rect_ui16_t WI_SWITCH_t<SZ>::getRollingRect(Iwindow_menu_t &window_menu, rect_ui16_t rect) const {
    return getRollingSpinRects(window_menu, rect)[0];
}

template <size_t SZ>
void WI_SWITCH_t<SZ>::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    std::array<rect_ui16_t, 2> rects = getRollingSpinRects(window_menu, rect);

    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.alignment);
    //draw spin
    render_text_align(rects[1], items[index], window_menu.font,
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
