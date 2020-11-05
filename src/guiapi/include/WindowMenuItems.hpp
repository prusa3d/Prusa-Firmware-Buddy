#pragma once

#include "IWindowMenuItem.hpp"
#include "GuiDefaults.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "super.hpp"
#include "i18n.h"
#include "menu_spin_config_type.hpp" //SpinConfig_t

#define UNIT_RECT_CHAR_WIDTH   4
#define UNIT_HALFSPACE_PADDING 6
#define SWITCH_ICON_RECT_WIDTH 40

//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no);
    virtual bool Change(int dif) override;
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const override;
    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override;
};

//IWiSpin
class IWiSpin : public AddSuper<IWindowMenuItem> {
protected:
    enum { WIO_MIN = 0,
        WIO_MAX = 1,
        WIO_STEP = 2 };
    static std::array<char, 10> temp_buff; //temporary buffer to print value for text measurements
    virtual void click(IWindowMenu &window_menu) final;
    // Old GUI implementation (is used in derived classes in printItem instead of derived func, when we want old GUI)
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual char *sn_prt() const = 0;

    /* getMenuRects (not derived because of New/Old GUI and 3 rect array return (not 2))
    *   Returns rect array according to this format: [label], [value], [units]
    */
    std::array<Rect16, 3> getSpinMenuRects(IWindowMenu &window_menu, Rect16 rect) const;

    /* derived printItem()
    * Adition of new implementation:
    *   values are offset from right (4 chars) and they are in secondary_font (smaller)
    *   optional value units are added next to values (gray color)
    */
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    IWiSpin(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : AddSuper<IWindowMenuItem>(label, id_icon, enabled, hidden) {}
    virtual void OnClick() {}
    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override;
};

//WI_SPIN
template <class T>
class WI_SPIN_t : public AddSuper<IWiSpin> {

public: //todo private
    using Config = SpinConfig_t<T>;
    //using var_t = std::aligned_union<4, float, uint32_t, int32_t>;
    //var_t value;
    T value;
    const Config &config;

protected:
    virtual char *sn_prt() const override;

public:
    WI_SPIN_t(T value, const Config &cnf, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual bool Change(int dif) override;
    void ClrVal() { value = static_cast<T>(0); }
};

using WI_SPIN_I08_t = WI_SPIN_t<int8_t>;
using WI_SPIN_I16_t = WI_SPIN_t<int16_t>;
using WI_SPIN_I32_t = WI_SPIN_t<int32_t>;
using WI_SPIN_U08_t = WI_SPIN_t<uint8_t>;
using WI_SPIN_U16_t = WI_SPIN_t<uint16_t>;
using WI_SPIN_U32_t = WI_SPIN_t<uint32_t>;
using WI_SPIN_FL_t = WI_SPIN_t<float>;

//todo inherit from WI_SPIN_t<const char**>
class IWiSwitch : public AddSuper<IWindowMenuItem> {
public:
    size_t index; //todo private
    IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden);
    virtual void ClrIndex() { index = 0; }
    virtual size_t size() = 0;
    virtual bool Change(int dif) override;
    virtual bool SetIndex(size_t idx);

    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override {
        reInitRoll(window_menu, getSwitchMenuRects(window_menu, rect)[0]);
    }

protected:
    virtual void OnChange(size_t old_index) = 0;
    virtual void click(IWindowMenu &window_menu) final;
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual const char *get_item() const = 0;

    /**
    * Returns Menu item rect devided into 4 rects according to this format: [label], ['['], [switch], [']']
    **/
    std::array<Rect16, 4> getSwitchMenuRects(IWindowMenu &window_menu, Rect16 rect) const;

    /* derived printItem()
    *  Aditions to the new implementation:
    *   every switch value is scoped in gray brackets []
    *   Secondary_font (smaller) is used on all items on right
    */
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;
};

/*****************************************************************************/
//WI_SWITCH == text version of WI_SPIN (non-numeric)
//unlike WI_SPIN cannot be selected
//todo try to inherit from WI_SPIN<const char**> lot of code could be reused
template <size_t SZ>
class WI_SWITCH_t : public AddSuper<IWiSwitch> {
public: //todo private
    const std::array<const char *, SZ> items;
    virtual size_t size() override { return items.size(); }

public:
    //cannot create const std::array<const char *, SZ> with std::initializer_list<const char*>
    //template<class ...E> and {{std::forward<E>(e)...}} is workaround
    template <class... E>
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden)
        , items { { std::forward<E>(e)... } } {}

protected:
    virtual const char *get_item() const override { return items[index]; }
};

//most common version of WI_SWITCH with on/off options
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = N_("Off");
    constexpr static const char *str_On = N_("On");

public:
    WI_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, str_Off, str_On) {}
};

template <size_t SZ>
class WI_ICON_SWITCH_t : public AddSuper<IWiSwitch> {
public: //todo private
    const std::array<uint16_t, SZ> items;
    virtual size_t size() override { return items.size(); }

public:
    template <class... E>
    WI_ICON_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden)
        , items { { std::forward<E>(e)... } } {}

    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override {
        reInitRoll(window_menu, getMenuRects(window_menu, rect)[0]);
    }

protected:
    virtual const char *get_item() const override { return 0; } // TODO: useless here (but we need virtual get_item in iWiSwitch)
    // Returns selected icon id
    const uint16_t get_icon() const { return items[index]; }
    // Returns array of rects in this format: [label], [icon]
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override {
        Rect16 base_rect = getRollingRect(window_menu, rect);
        Rect16 icon_rect = { 0, 0, 0, 0 };
        if (get_icon()) { // WI_ICON_SWITCH
            icon_rect = { int16_t(base_rect.Left() + base_rect.Width() - SWITCH_ICON_RECT_WIDTH), base_rect.Top(), SWITCH_ICON_RECT_WIDTH, base_rect.Height() };
        }
        base_rect -= icon_rect.Width();
        return std::array<Rect16, 2> { base_rect, icon_rect };
    }

    /*
    * derived printItem()
    *   prints label and selected icon
    */
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override {
        std::array<Rect16, 2> rects = getMenuRects(window_menu, rect);
        //draw label
        printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
        //draw on/off icon
        render_icon_align(rects[1], get_icon(), window_menu.color_back, RENDER_FLG(ALIGN_CENTER, swap));
    }
};

class WI_ICON_SWITCH_OFF_ON_t : public WI_ICON_SWITCH_t<2> {
    constexpr static const uint16_t iid_off = IDR_PNG_switch_off_36px;
    constexpr static const uint16_t iid_on = IDR_PNG_switch_on_36px;

public:
    WI_ICON_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_ICON_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, iid_off, iid_on) {}
};

//currently broken todo FIXME
//WI_SELECT == switch with no label
//but can be selected like WI_SPIN
class WI_SELECT_t : public IWindowMenuItem {
public: //todo private
    uint32_t index;
    const char **strings;

protected:
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual bool Change(int dif) override;
};

/*****************************************************************************/
//template definitions
//WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T value, const Config &cnf, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : AddSuper<IWiSpin>(label, id_icon, enabled, hidden)
    , value(value)
    , config(cnf) {}

template <class T>
bool WI_SPIN_t<T>::Change(int dif) {
    T old = value;
    value += (T)dif * config.range[WIO_STEP];
    value = dif >= 0 ? std::max(value, old) : std::min(value, old); //check overflow/underflow
    value = std::min(value, config.range[WIO_MAX]);
    value = std::max(value, config.range[WIO_MIN]);
    return old != value;
}

template <class T>
char *WI_SPIN_t<T>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), config.prt_format, value);
    return temp_buff.data();
}

template <>
inline char *WI_SPIN_t<float>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), config.prt_format, static_cast<double>(value));
    return temp_buff.data();
}

/*****************************************************************************/
//template definitions

/*****************************************************************************/
//advanced types
class MI_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Return");

public:
    MI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};

class MI_TEST_DISABLED_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Disabled RETURN button";

public:
    MI_TEST_DISABLED_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};
