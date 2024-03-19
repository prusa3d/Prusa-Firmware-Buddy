/**
 * @file WindowMenuSpin.hpp
 * @author Radek Vana
 * @brief Spinner menu item, allow to numeric change value
 * @date 2020-11-09
 */

#pragma once

#include "i_window_menu_item.hpp"
#include "i18n.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <printers.h>

/*****************************************************************************/
// IWiSpin
class IWiSpin : public IWindowMenuItem {
protected:
    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSpinHasUnits ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPaddingItems;
    static constexpr size_t unit__half_space_padding = 6;
    static constexpr bool has_unit = GuiDefaults::MenuSpinHasUnits;

    using SpinTextArray = std::array<char, 10>;
    SpinTextArray spin_text_buff; // temporary buffer to print value for text measurements

    string_view_utf8 units;

    static Rect16::Width_t calculateExtensionWidth(const string_view_utf8 &units, size_t value_max_digits);
    Rect16 getSpinRect(Rect16 extension_rect) const;
    Rect16 getUnitRect(Rect16 extension_rect) const;

    void click(IWindowMenu &window_menu) override;
    void touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) final;
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;

public:
    IWiSpin(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, string_view_utf8 units_, size_t extension_width_);
    virtual void OnClick() {}
};

enum class SpinUnit : uint8_t {
    none,
    percent,
    celsius,
    hour,
    second,
    hertz,
    millimeter,
    micrometer,
    milliamper,
};

enum class spin_off_opt_t : bool { no,
    yes }; // yes == lowest value is off

template <class T>
struct SpinConfig {
    std::array<T, 3> range; // todo change array to struct containing min, max, step
    const char *const prt_format;
    spin_off_opt_t off_opt;
    const SpinUnit unit;
    static constexpr const char *const off_opt_str = N_("Off");

    constexpr SpinConfig(const std::array<T, 3> &arr, const SpinUnit unit_ = SpinUnit::none, spin_off_opt_t off_opt_ = spin_off_opt_t::no, const char *const format_override = nullptr)
        : range(arr)
        , prt_format([format_override] {
            if (format_override) {
                return format_override;
            }
            if constexpr (std::is_same_v<T, int>) {
                return "%d";
            }
            if constexpr (std::is_same_v<T, int>) {
                return "%f";
            }
            return "";
        }())
        , off_opt(off_opt_)
        , unit(unit_) {}

#if PRINTER_IS_PRUSA_MINI
    // MINI does not have screen space for units
    constexpr const char *Unit() const { return nullptr; }
#else
    constexpr const char *Unit() const {
        // Note: These do not need translation
        switch (unit) {
        case SpinUnit::none:
            return "";
        case SpinUnit::percent:
            return "%";
        case SpinUnit::celsius:
            return "\xC2\xB0\x43"; // degree Celsius
        case SpinUnit::hour:
            return "h";
        case SpinUnit::second:
            return "s";
        case SpinUnit::hertz:
            return "Hz";
        case SpinUnit::millimeter:
            return "mm";
        case SpinUnit::micrometer:
            return "um"; //"µm";
        case SpinUnit::milliamper:
            return "mA";
        }
        abort();
    }
#endif
    constexpr T clamp(T value) const { return std::clamp(value, Min(), Max()); }

    constexpr T Min() const { return range[0]; }
    constexpr T Max() const { return range[1]; }
    constexpr T Step() const { return range[2]; }
    bool IsOffOptionEnabled() const { return off_opt == spin_off_opt_t::yes; }

    size_t snprintf(char *data, size_t size, T val) const {
        // Variadic function ABI requires promotion of floats to doubles.
        // We ignore diagnostics here in order to not have multiple specializations.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
        return ::snprintf(data, size, prt_format, val);
#pragma GCC diagnostic pop
    }

    size_t compute_max_chars() const {
        return std::max<size_t>({
            IsOffOptionEnabled() ? _(off_opt_str).computeNumUtf8CharsAndRewind() : 0ul,
            snprintf(nullptr, 0, Max()),
            snprintf(nullptr, 0, Min()),
        });
    }
};

// TODO find a better way to share these constants...
static constexpr SpinConfig<int> print_progress_spin_config {
#if PRINTER_IS_PRUSA_MINI
    { 30, 200, 1 }
#else
    // 29 because spin_off_opt_t::yes uses lowest value to represent off...
    { 29, 200, 1 }, SpinUnit::second, spin_off_opt_t::yes,
#endif
};

/*****************************************************************************/
// WI_SPIN_t
template <class T>
class WI_SPIN_t : public IWiSpin {
private:
    T value;

public: // todo private
    using Config = SpinConfig<T>;
    const Config &config;

private:
    void update() {
        if (config.IsOffOptionEnabled() && value == config.Min()) {
            _(config.off_opt_str).copyToRAM(spin_text_buff.data(), spin_text_buff.size());
        } else {
            config.snprintf(spin_text_buff.data(), spin_text_buff.size(), value);
        }
    }

protected:
    virtual invalidate_t change(int dif) override {
        T val = GetVal();
        T old = val;
        val += (T)dif * config.Step();
        val = dif >= 0 ? std::max(val, old) : std::min(val, old); // check overflow/underflow
        val = config.clamp(val);
        set_val(val);
        if (!dif || old != val) { // 0 dif forces redraw
            update();
            return invalidate_t::yes;
        } else {
            return invalidate_t::no;
        }
    }
    void set_val(T val) { value = val; }

public:
    WI_SPIN_t(T val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : IWiSpin(label, id_icon, enabled, hidden,
            cnf.Unit() == nullptr ? string_view_utf8::MakeNULLSTR() : _(cnf.Unit()), 0)
        , value { cnf.clamp(val) }
        , config(cnf) {
        extension_width = calculateExtensionWidth(units, config.compute_max_chars());
        update();
    }

    T GetVal() const { return value; }

    /**
     * @brief Set value.
     * validates data with Change(0)
     */
    inline void SetVal(T val) {
        set_val(val);
        Change(0);
    }
};

using WiSpinInt = WI_SPIN_t<int>;
using WiSpinFlt = WI_SPIN_t<float>;
