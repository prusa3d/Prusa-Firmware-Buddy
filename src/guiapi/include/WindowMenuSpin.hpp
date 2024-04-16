/**
 * @file WindowMenuSpin.hpp
 * @author Radek Vana
 * @brief Spinner menu item, allow to numeric change value
 * @date 2020-11-09
 */

#pragma once

#include "i_window_menu_item.hpp"
#include "menu_spin_config_type.hpp" //SpinConfig_t

#include "feature/tmc_util.h"

/*****************************************************************************/
// IWiSpin
class IWiSpin : public IWindowMenuItem {
    SpinType value;

protected:
    /**
     * @brief returns the same type to be on the safe side (SpinType is not type safe)
     *
     * @tparam T int or float
     * @return T member of union
     */
    template <class T>
    T get_val() const { return value; }

    /**
     * @brief Set value.
     * intentionally does not validate data with Change(0)
     * @tparam T int or float
     * @param val new value of the correct type (SpinType is not type safe)
     */
    template <class T>
    inline void set_val(T val) {
        value = val;
    }

    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSpinHasUnits ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPaddingItems;
    static constexpr size_t unit__half_space_padding = 6;
    static constexpr bool has_unit = GuiDefaults::MenuSpinHasUnits;

    using SpinTextArray = std::array<char, 10>;
    SpinTextArray spin_text_buff; // temporary buffer to print value for text measurements

    string_view_utf8 units;
    size_t spin_val_width;

    static Rect16::Width_t calculateExtensionWidth(size_t unit_len, unichar uchar, size_t value_max_digits);
    Rect16 getSpinRect(Rect16 extension_rect) const;
    Rect16 getUnitRect(Rect16 extension_rect) const;
    void changeExtentionWidth(size_t unit_len, unichar uchar, size_t width);

    void click(IWindowMenu &window_menu) override;
    void touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) final;
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;

public:
    IWiSpin(SpinType val, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, string_view_utf8 units_, size_t extension_width_);
    virtual void OnClick() {}

    /// don't define GetVal nor SetVal here since we don't know the type yet
    /// and C++ does not allow return type overloading (yet)
};

/*****************************************************************************/
// WI_SPIN_t
template <class T>
class WI_SPIN_t : public IWiSpin {

public: // todo private
    using Config = SpinConfig_t<T>;
    const Config &config;

protected:
    void printSpinToBuffer();
    virtual invalidate_t change(int dif) override;

public:
    WI_SPIN_t(T val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);

    /// returns the same type to be on the safe side (SpinType is not type safe)
    T GetVal() const { return get_val<T>(); }

    /**
     * @brief Set value.
     * validates data with Change(0)
     * @param val new value of the correct type (SpinType is not type safe)
     */
    inline void SetVal(T val) {
        set_val(val);
        Change(0);
    }
};

/*****************************************************************************/
// template definitions
// WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWiSpin(std::clamp(T(val), cnf.Min(), cnf.Max()), label, id_icon, enabled, hidden,
        cnf.Unit() == nullptr ? string_view_utf8::MakeNULLSTR() : _(cnf.Unit()), 0)
    , config(cnf) {
    printSpinToBuffer();

    spin_val_width = cnf.txtMeas(val);
    size_t unit_len = 0;
    unichar uchar = 0;
    if (config.Unit() != nullptr) {
        string_view_utf8 un = units;
        uchar = un.getUtf8Char();
        un.rewind();
        unit_len = un.computeNumUtf8CharsAndRewind();
    }
    extension_width = calculateExtensionWidth(unit_len, uchar, spin_val_width);
}

template <class T>
invalidate_t WI_SPIN_t<T>::change(int dif) {
    T val = GetVal();
    T old = val;
    val += (T)dif * config.Step();
    val = dif >= 0 ? std::max(val, old) : std::min(val, old); // check overflow/underflow
    val = std::clamp(val, config.Min(), config.Max());
    set_val(val);
    invalidate_t invalid = (!dif || old != val) ? invalidate_t::yes : invalidate_t::no; // 0 dif forces redraw
    if (invalid == invalidate_t::yes) {
        if (!has_unit || config.Unit() == nullptr) {
            changeExtentionWidth(0, 0, config.txtMeas(GetVal()));
        } else {
            string_view_utf8 un = units;
            unichar uchar = un.getUtf8Char();
            un.rewind();
            changeExtentionWidth(units.computeNumUtf8CharsAndRewind(), uchar, config.txtMeas(GetVal()));
        }
        printSpinToBuffer(); // could be in draw method, but traded little performance for code size (printSpinToBuffer is not virtual when it is here)
    }
    return invalid;
}

template <class T>
void WI_SPIN_t<T>::printSpinToBuffer() {
    if (config.IsOffOptionEnabled() && GetVal() == config.Min()) {
        _(config.off_opt_str).copyToRAM(spin_text_buff.data(), _(config.off_opt_str).computeNumUtf8CharsAndRewind() + 1);
    } else {
        snprintf(spin_text_buff.data(), spin_text_buff.size(), config.prt_format_override ? config.prt_format_override : config.prt_format, GetVal());
    }
}

template <>
inline void WI_SPIN_t<float>::printSpinToBuffer() {
    snprintf(spin_text_buff.data(), spin_text_buff.size(), config.prt_format_override ? config.prt_format_override : config.prt_format, static_cast<double>(GetVal()));
}

using WiSpinInt = WI_SPIN_t<int>;
using WiSpinFlt = WI_SPIN_t<float>;

#include "../../../lib/Marlin/Marlin/src/feature/prusa/crash_recovery.hpp"
#if ENABLED(CRASH_RECOVERY)

class WI_SPIN_CRASH_PERIOD_t : public IWiSpin {

public: // todo private
    using Config = SpinConfig_t<int>;
    const Config &config;

protected:
    void printSpinToBuffer() {
        float display = tmc_period_to_feedrate(X_AXIS, get_microsteps_x(), get_val<int>(), get_steps_per_unit_x());
        int chars = snprintf(spin_text_buff.data(), spin_text_buff.size(), "%f", double(display));
        changeExtentionWidth(0, 0, std::min<int>(chars, spin_text_buff.size() - 1));
    }

public:
    WI_SPIN_CRASH_PERIOD_t(int val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual invalidate_t change(int dif) override;
    /// returns the same type to be on the safe side (SpinType is not type safe)
    int GetVal() const { return get_val<int>(); }
};

#endif
