/**
 * @file WindowMenuSpin.hpp
 * @author Radek Vana
 * @brief Spinner menu item, allow to numeric change value
 * @date 2020-11-09
 */

#pragma once

#include "WindowMenuLabel.hpp"
#include "menu_spin_config_type.hpp" //SpinConfig_t

/*****************************************************************************/
//IWiSpin
class IWiSpin : public AddSuper<WI_LABEL_t> {
protected:
    static constexpr font_t *&Font = GuiDefaults::MenuSpinHasUnits ? GuiDefaults::FontMenuSpecial : GuiDefaults::FontMenuItems;
    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSpinHasUnits ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPadding;
    static constexpr size_t unit__half_space_padding = 6;
    static constexpr bool has_unit = GuiDefaults::MenuSpinHasUnits;

    using SpinTextArray = std::array<char, 10>;
    SpinTextArray spin_text_buff; //temporary buffer to print value for text measurements

    string_view_utf8 units;
    SpinType value;

    static Rect16::Width_t calculateExtensionWidth(const char *unit, size_t value_max_digits);
    Rect16 getSpinRect(Rect16 extension_rect) const;
    Rect16 getUnitRect(Rect16 extension_rect) const;

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
