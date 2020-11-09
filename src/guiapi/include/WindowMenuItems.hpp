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
#include "window_icon.hpp" //CalculateMinimalSize

//IWindowMenuItem
//todo make version with constant label
//layouts
//+-------+-----------------------------+
//| icon  | text                        | label
//+-------+--------------+--------------+
//| icon  | text         | arrow        | label with expand
//+-------+--------------+--------------+
//| icon  | text         | value        | spin
//+-------+--------------+-------+------+
//| icon  | text         | value | unit | spin with units
//+-------+--------------+-------+------+
//| icon  | text         | value        |
//+-------+--------------+--------------+
//| icon  | text         | [value]      | switch with brackets
//+-------+--------------+--------------+

/*****************************************************************************/
//IWindowMenuItem
class IWindowMenuItem {
protected:
    //could me moved to gui defaults
    static constexpr Rect16::Width_t expand_icon_width = 26;
    static constexpr Rect16::Width_t icon_width = 26;

private:
    string_view_utf8 label;
    txtroll_t roll;

    is_hidden_t hidden : 1;
    is_enabled_t enabled : 1;
    is_focused_t focused : 1;

protected:
    is_selected_t selected : 1; // should be in IWiSpin, but is here because of size optimization
    bool has_brackets : 1;      // determines if Switch has brackets, should be in IWiSwitch, but is here because of size optimization
    bool has_unit : 1;          // determines if Spin has units, should be in IWiSpin, but is here because of size optimization
    uint16_t id_icon : 10;
    Rect16::Width_t extension_width;

    static Rect16 getCustomRect(Rect16 base_rect, uint16_t custom_rect_width); // general method Returns custom width Rectangle, aligned intersection on the right of the base_rect
    Rect16 getIconRect(Rect16 rect) const;
    Rect16 getLabelRect(Rect16 rect) const;
    Rect16 getExtensionRect(Rect16 rect) const;

    virtual void printIcon(Rect16 icon_rect, uint8_t swap, color_t color_back) const; //must be virtual, because pictures of flags are drawn differently
    void printLabel(Rect16 label_rect, color_t color_text, color_t color_back) const;

    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const; //things behind rect
    virtual void click(IWindowMenu &window_menu) = 0;

    void reInitRoll(Rect16 rect);

public:
    IWindowMenuItem(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no);
    IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual ~IWindowMenuItem() = default;
    void Enable() { enabled = is_enabled_t::yes; }
    void Disable() { enabled = is_enabled_t::no; }
    bool IsEnabled() const { return enabled == is_enabled_t::yes; }
    bool IsSelected() const { return selected == is_selected_t::yes; }
    void Hide() { hidden = is_hidden_t::yes; }
    void Show() { hidden = is_hidden_t::no; }
    bool IsHidden() const { return hidden == is_hidden_t::yes; }
    void SetFocus();
    void ClrFocus();
    bool IsFocused() const { return focused == is_focused_t::yes; }
    void SetIconId(uint16_t id) { id_icon = id; }
    uint16_t GetIconId() const { return id_icon; }
    inline void SetLabel(string_view_utf8 text) { label = text; }
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    inline string_view_utf8 GetLabel() const { return label; }

    void Print(Rect16 rect) const;

    inline invalidate_t Increment(uint8_t dif) { return Change(dif); }
    inline invalidate_t Decrement(uint8_t dif) { return Change(-int(dif)); }
    void Click(IWindowMenu &window_menu);
    inline void InitRollIfNeeded(Rect16 rect) { reInitRoll(getLabelRect(rect)); }
    virtual invalidate_t Change(int /*dif*/) { return invalidate_t::no; }
    inline invalidate_t Roll() { return roll.Tick(); }
};

/*****************************************************************************/
//WI_LABEL_t
using WI_LABEL_t = IWindowMenuItem;

/*****************************************************************************/
//IWiSpin
class IWiSpin : public AddSuper<WI_LABEL_t> {
protected:
    static constexpr size_t unit__half_space_padding = 6;

    using SpinTextArray = std::array<char, 10>;
    SpinTextArray spin_text_buff; //temporary buffer to print value for text measurements

    string_view_utf8 units;
    SpinType value;

    static Rect16::Width_t calculateExtensionWidth(const char *unit, size_t value_max_digits) {
        string_view_utf8 un = _(unit);
        size_t ret = unit == nullptr ? 0 : un.computeNumUtf8CharsAndRewind() * GuiDefaults::FontMenuSpecial->w;
        ret += value_max_digits * (unit ? GuiDefaults::FontMenuItems->w : GuiDefaults::FontMenuSpecial->w);
        return ret;
    }
    Rect16 getSpinRect(Rect16 rect) const;
    Rect16 getUnitRect(Rect16 rect) const;

    virtual void click(IWindowMenu &window_menu) final;
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    IWiSpin(SpinType val, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, string_view_utf8 units_, size_t extension_width_);
    virtual void OnClick() {}
    inline void ClrVal() { value.u32 = 0; }
    inline void SetVal(SpinType val) { value = val; }
    inline SpinType GetVal() const { return value; }
};

/*****************************************************************************/
//WI_SPIN_t
template <class T>
class WI_SPIN_t : public AddSuper<IWiSpin> {

public: //todo private
    using Config = SpinConfig_t<T>;
    const Config &config;

protected:
    void printSpinToBuffer();

public:
    WI_SPIN_t(T val, const Config &cnf, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual invalidate_t Change(int dif) override;
};

/*****************************************************************************/
//template definitions
//WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T val, const Config &cnf, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : AddSuper<IWiSpin>(val, label, id_icon, enabled, hidden, cnf.Unit() == nullptr ? string_view_utf8::MakeNULLSTR() : _(cnf.Unit()), calculateExtensionWidth(cnf.Unit(), cnf.calculateMaxDigits()))
    , config(cnf) {
    printSpinToBuffer();
}

template <class T>
invalidate_t WI_SPIN_t<T>::Change(int dif) {
    T val = (T)value;
    T old = val;
    val += (T)dif * config.Step();
    val = dif >= 0 ? std::max(val, old) : std::min(val, old); //check overflow/underflow
    val = std::min(val, config.Max());
    val = std::max(val, config.Min());
    value = val;
    if (old != val)
        printSpinToBuffer(); // could be in draw method, but traded little performance for code size (printSpinToBuffer is not virtual when it is here)
    return old == val ? invalidate_t::no : invalidate_t::yes;
}

template <class T>
void WI_SPIN_t<T>::printSpinToBuffer() {
    snprintf(spin_text_buff.data(), spin_text_buff.size(), config.prt_format, (T)(value));
}

template <>
inline void WI_SPIN_t<float>::printSpinToBuffer() {
    snprintf(spin_text_buff.data(), spin_text_buff.size(), config.prt_format, static_cast<double>(value.flt));
}

using WI_SPIN_I08_t = WI_SPIN_t<int8_t>;
using WI_SPIN_I16_t = WI_SPIN_t<int16_t>;
using WI_SPIN_I32_t = WI_SPIN_t<int32_t>;
using WI_SPIN_U08_t = WI_SPIN_t<uint8_t>;
using WI_SPIN_U16_t = WI_SPIN_t<uint16_t>;
using WI_SPIN_U32_t = WI_SPIN_t<uint32_t>;
using WI_SPIN_FL_t = WI_SPIN_t<float>;

/*****************************************************************************/
//IWiSwitch
class IWiSwitch : public AddSuper<WI_LABEL_t> {
protected:
    size_t index;

public:
    struct Items_t {
        const char *const *texts;
        size_t size;
    };

    static constexpr font_t *&BracketFont = GuiDefaults::FontMenuSpecial;

    IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, size_t extension_width_);

    virtual invalidate_t Change(int dif) override;
    bool SetIndex(size_t idx);

protected:
    static Rect16::Width_t calculateExtensionWidth(size_t max_switch_characters) {
        size_t ret = GuiDefaults::FontMenuItems->w * max_switch_characters + (GuiDefaults::MenuSwitchHasBrackets ? BracketFont->w * 2 : 0);
        return ret;
    }

    virtual void OnChange(size_t old_index) = 0;
    virtual Items_t get_items() const = 0;
    virtual void click(IWindowMenu &window_menu) final;

    Rect16 getSwitchRect(Rect16 extension_rect) const;
    Rect16 getLeftBracketRect(Rect16 extension_rect) const;
    Rect16 getRightBracketRect(Rect16 extension_rect) const;

    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const override;
};

/*****************************************************************************/
//WI_SWITCH == text version of WI_SPIN (non-numeric)
//unlike WI_SPIN cannot be selected
//todo try to inherit from WI_SPIN<const char**> lot of code could be reused
template <size_t SZ>
class WI_SWITCH_t : public AddSuper<IWiSwitch> {
public: //todo private
    //using Array = const std::array<string_view_utf8, SZ>;//do not erase this commented code, solution storing string_view_utf8 instead const char*
    using Array = const std::array<const char *, SZ>;
    const Array items;

    //cannot create const std::array<const char *, SZ> with std::initializer_list<const char*>
    //template<class ...E> and {{std::forward<E>(e)...}} is workaround
    template <class... E>
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden, calculateExtensionWidth(Array({ std::forward<E>(e)... })))
        , items { { std::forward<E>(e)... } } {}

    //do not erase this commented code, solution storing string_view_utf8 instead const char*
    /*
    static size_t MaxNumberOfCharacters(const Array>& items) {
        size_t max_len = 0;
        for (size_t i = 0; i < SZ; ++i) {
            size_t len = items[i].computeNumUtf8CharsAndRewind();
            if (len > max_len) max_len = len;
        }
        return IWiSwitch::calculateExtensionWidth(max_len);
    }

    //ctor must receive translated texts
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden)
        , items { { std::forward<E>(e)... } } {}
    */
protected:
    static size_t calculateExtensionWidth(const Array &items) {
        size_t max_len = 0;
        for (size_t i = 0; i < SZ; ++i) {
            string_view_utf8 label = _(items[i]);
            size_t len = label.computeNumUtf8CharsAndRewind();
            if (len > max_len)
                max_len = len;
        }
        return IWiSwitch::calculateExtensionWidth(max_len);
    }

    virtual Items_t get_items() const override {
        const Items_t ret = { items.data(), items.size() };
        return ret;
    }
};

// most common version of WI_SWITCH with on/off options
// also very nice how-to-use example
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = N_("Off");
    constexpr static const char *str_On = N_("On");

public:
    WI_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, str_Off, str_On) {}
};

template <size_t SZ>
class WI_ICON_SWITCH_t : public AddSuper<IWiSwitch> {
protected:
    using Array = const std::array<uint16_t, SZ>;
    const Array items;

    static size_t calculateExtensionWidth(const Array &items) {
        size_t max_width = 0;
        for (size_t i = 0; i < SZ; ++i) {
            size_t width = window_icon_t::CalculateMinimalSize(items[i]).w;
            if (width > max_width)
                max_width = width;
        }
        return max_width;
    }

public:
    template <class... E>
    WI_ICON_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden, calculateExtensionWidth(Array({ std::forward<E>(e)... })))
        , items { { std::forward<E>(e)... } } {}

protected:
    virtual Items_t get_items() const override {
        const Items_t ret = { nullptr, items.size() };
        return ret;
    }
    // Returns selected icon id
    const uint16_t get_icon() const { return items[index]; }

    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const override {
        //draw on/off icon
        render_icon_align(extension_rect, get_icon(), GuiDefaults::MenuColorBack, RENDER_FLG(ALIGN_CENTER, swap));
    }
};

class WI_ICON_SWITCH_OFF_ON_t : public WI_ICON_SWITCH_t<2> {
    constexpr static const uint16_t iid_off = IDR_PNG_switch_off_36px;
    constexpr static const uint16_t iid_on = IDR_PNG_switch_on_36px;

public:
    WI_ICON_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_ICON_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, iid_off, iid_on) {}
};

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
