#pragma once

#include "IWindowMenuItem.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "../lang/i18n.h"

//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif) override;
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
    void sn_prt(char *buff, size_t len) const;
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
template <size_t SZ>
class WI_SWITCH_t : public IWindowMenuItem {
public: //todo private
    size_t index;
    const std::array<const char *, SZ> items;

protected:
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const override;
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
    virtual bool SetIndex(size_t idx);

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
    : IWindowMenuItem(label, id_icon, enabled, hidden)
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

template <class T>
void WI_SPIN_t<T>::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    char buff[20] = { '\0' };
    sn_prt(buff, 20);

    rect_ui16_t spin_rect = {
        uint16_t(rect.x + rect.w), rect.y, uint16_t(window_menu.font->w * strlen(buff) + window_menu.padding.left + window_menu.padding.right), rect.h
    };
    spin_rect.x -= spin_rect.w;
    rect.w -= spin_rect.w;

    rect_ui16_t label_rect = rect;
    label_rect.w = spin_rect.x - label_rect.x;

    //draw label
    IWindowMenuItem::printText(window_menu, label_rect, color_text, color_back, swap);
    //draw spin
    render_text_align(spin_rect, _(buff), window_menu.font,
        color_back, IsSelected() ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.alignment);
}

template <class T>
void WI_SPIN_t<T>::sn_prt(char *buff, size_t len) const {
    snprintf(buff, len, prt_format, value);
}

template <>
inline void WI_SPIN_t<float>::sn_prt(char *buff, size_t len) const {
    snprintf(buff, len, prt_format, static_cast<double>(value));
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

template <size_t SZ>
void WI_SWITCH_t<SZ>::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    IWindowMenuItem::printText(window_menu, rect, color_text, color_back, swap);
    const char *txt = items[index];

    rect_ui16_t vrc = {
        uint16_t(rect.x + rect.w), rect.y, uint16_t(window_menu.font->w * strlen(txt) + window_menu.padding.left + window_menu.padding.right), rect.h
    };
    vrc.x -= vrc.w;
    rect.w -= vrc.w;

    render_text_align(vrc, _(txt), window_menu.font,
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
