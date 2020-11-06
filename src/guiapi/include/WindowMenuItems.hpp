#pragma once

#include "GuiDefaults.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "super.hpp"
#include "i18n.h"
#include "menu_spin_config_type.hpp" //SpinConfig_t
#include "Iwindow_menu.hpp"          //needed for window settings like rect, padding ...
#include "text_roll.hpp"

#define UNIT_RECT_CHAR_WIDTH   4
#define UNIT_HALFSPACE_PADDING 6
#define SWITCH_ICON_RECT_WIDTH 40

//IWindowMenuItem
//todo make version with constant label
//layouts
//+-------+--------------+--------------+
//| icon  | text                        | label
//+-------+--------------+--------------+
//| icon  | text         | arrow        | label with expand
//+-------+--------------+--------------+
//| icon  | text         | value        | spin
//+-------+--------------+--------------+
//| icon  | text         | value        | spin with units
//+-------+--------------+-------+------+
//| icon  | text         | value | unit | switch
//+-------+--------------+-------+------+
//| icon  | text         | [value]      | switch with brackets
//+-------+--------------+--------------+

class IWindowMenuItem {
    string_view_utf8 label;
    txtroll_t roll;

    is_hidden_t hidden : 1;
    is_enabled_t enabled : 1;
    is_focused_t focused : 1;

protected:
    is_selected_t selected : 1; //should be in child, but is here because of size optimization
    expands_t expands : 1;      // determines if Label expands to another screen (with a click)
    bool has_brackets : 1;      // determines if Switch has brackets
    uint16_t id_icon : 10;

    virtual void printIcon(IWindowMenu &window_menu, Rect16 rect, uint8_t swap, color_t color_back) const;
    void printLabel_into_rect(Rect16 rolling_rect, color_t color_text, color_t color_back, const font_t *font, padding_ui8_t padding, uint8_t alignment) const;
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const;
    virtual void click(IWindowMenu &window_menu) = 0;
    virtual Rect16 getRollingRect(IWindowMenu &window_menu, Rect16 rect) const;
    static Rect16 getCustomRect(IWindowMenu &window_menu, Rect16 base_rect, uint16_t custom_rect_width);
    static Rect16 getIconRect(IWindowMenu &window_menu, Rect16 rect);
    void reInitRoll(IWindowMenu &window_menu, Rect16 rect);

public:
    IWindowMenuItem(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no);
    virtual ~IWindowMenuItem() = default;
    void Enable() { enabled = is_enabled_t::yes; }
    void Disable() { enabled = is_enabled_t::no; }
    bool IsEnabled() const { return enabled == is_enabled_t::yes; }
    void Hide() { hidden = is_hidden_t::yes; }
    void Show() { hidden = is_hidden_t::no; }
    bool IsHidden() const { return hidden == is_hidden_t::yes; }
    void SetFocus();
    void ClrFocus();
    bool IsFocused() const { return focused == is_focused_t::yes; }
    void SetIconId(uint16_t id) { id_icon = id; }
    uint16_t GetIconId() const { return id_icon; }
    void SetLabel(string_view_utf8 text);
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    string_view_utf8 GetLabel() const;

    void Print(IWindowMenu &window_menu, Rect16 rect) const;

    bool IsSelected() const { return selected == is_selected_t::yes; }
    virtual bool Change(int dif);
    inline bool Increment(uint8_t dif) { return Change(dif); }
    inline bool Decrement(uint8_t dif) { return Change(-int(dif)); }
    void Click(IWindowMenu &window_menu);
    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect);
    invalidate_t Roll();

    std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const;
};
using WI_LABEL_t = IWindowMenuItem;

//IWiSpin
class IWiSpin : public AddSuper<WI_LABEL_t> {
protected:
    SpinType value;
    static std::array<char, 10> temp_buff; //temporary buffer to print value for text measurements
    virtual void click(IWindowMenu &window_menu) final;
    // Old GUI implementation (is used in derived classes in printItem instead of derived func, when we want old GUI)
    std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const;
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
    IWiSpin(SpinType val, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : AddSuper<WI_LABEL_t>(label, id_icon, enabled, hidden)
        , value(val) {}
    virtual void OnClick() {}
    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override;
    inline void ClrVal() { value.u32 = 0; }
    inline void SetVal(SpinType val) { value = val; }
    inline SpinType GetVal() const { return value; }
};

//WI_SPIN
template <class T>
class WI_SPIN_t : public AddSuper<IWiSpin> {

public: //todo private
    using Config = SpinConfig_t<T>;
    const Config &config;

protected:
    virtual char *sn_prt() const override;

public:
    WI_SPIN_t(T value, const Config &cnf, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual bool Change(int dif) override;
};

using WI_SPIN_I08_t = WI_SPIN_t<int8_t>;
using WI_SPIN_I16_t = WI_SPIN_t<int16_t>;
using WI_SPIN_I32_t = WI_SPIN_t<int32_t>;
using WI_SPIN_U08_t = WI_SPIN_t<uint8_t>;
using WI_SPIN_U16_t = WI_SPIN_t<uint16_t>;
using WI_SPIN_U32_t = WI_SPIN_t<uint32_t>;
using WI_SPIN_FL_t = WI_SPIN_t<float>;

//todo inherit from WI_SPIN_t<const char**>
class IWiSwitch : public AddSuper<WI_LABEL_t> {
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
    std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const;
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
    std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
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
class WI_SELECT_t : public WI_LABEL_t {
public: //todo private
    uint32_t index;
    const char **strings;

protected:
    std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const;
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual bool Change(int dif) override;
};

/*****************************************************************************/
//template definitions
//WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T val, const Config &cnf, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : AddSuper<IWiSpin>(val, label, id_icon, enabled, hidden)
    , config(cnf) {}

template <class T>
bool WI_SPIN_t<T>::Change(int dif) {
    T val = (T)value;
    T old = val;
    val += (T)dif * config.Step();
    val = dif >= 0 ? std::max(val, old) : std::min(val, old); //check overflow/underflow
    val = std::min(val, config.Max());
    val = std::max(val, config.Min());
    value = val;
    return old != val;
}

template <class T>
char *WI_SPIN_t<T>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), config.prt_format, (T)(value));
    return temp_buff.data();
}

template <>
inline char *WI_SPIN_t<float>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), config.prt_format, static_cast<double>(value.flt));
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
