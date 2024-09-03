/**
 * @file WindowMenuSpin.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuSpin.hpp"

#include <str_utils.hpp>

#if HAS_TOUCH()
    #include <dialog_numeric_input.hpp>
    #include <gui/event/touch_event.hpp>
#endif

WiSpin::WiSpin(float value, const NumericInputConfig &config, const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWindowMenuItem(label, calculateExtensionWidth(config), id_icon, enabled, hidden)
    , config_(config)
    , value_(value) //
{
    update();
}

void WiSpin::click(IWindowMenu & /*window_menu*/) {
    if (is_edited()) {
        OnClick();
    }
    toggle_edit_mode();
}

void WiSpin::event(WindowMenuItemEventContext &ctx) {
#if HAS_TOUCH()
    if (auto e = ctx.event.value_maybe<gui_event::TouchEvent>()) {
        if (is_touch_in_extension_rect(*ctx.menu, e->relative_touch_point)) {
            const auto r = DialogNumericInput::exec(GetLabel(), value(), config_);
            if (r.has_value()) {
                set_value(*r);
                OnClick();
            }
        }

        // Accept touch in every case - we don't want the event to keep propagating
        ctx.accept();

        // Return - we don't want the default 'click' behavior from IWindowMenuItem
        return;
    }
#endif

    IWindowMenuItem::event(ctx);
}

Rect16 WiSpin::getSpinRect(Rect16 extension_rect) const {
    extension_rect -= getUnitRect(extension_rect).Width();
    return extension_rect;
}

Rect16 WiSpin::getUnitRect(Rect16 extension_rect) const {
    Rect16 ret = extension_rect;
    if (show_units && config_.unit != Unit::none) {
        const auto unit = config_.unit_str();
        const unichar uchar = unit.getFirstUtf8Char();
        size_t half_space_padding = (uchar == 0 || uchar == 0xB0) ? 0 : unit__half_space_padding;
        Rect16::Width_t unit_width = unit.computeNumUtf8Chars() * width(GuiDefaults::FontMenuSpecial) + Rect16::Width_t(half_space_padding);
        unit_width = unit_width + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        ret = unit_width;
    } else {
        ret = Rect16::Width_t(0);
    }
    ret += Rect16::Left_t(extension_rect.Width() - ret.Width());
    return ret;
}

static constexpr Font TheFont = GuiDefaults::MenuSpinHasUnits ? GuiDefaults::FontMenuSpecial : GuiDefaults::FontMenuItems;

void WiSpin::printExtension(Rect16 extension_rect, Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {

    const string_view_utf8 spin_txt = string_view_utf8::MakeRAM(text_buffer_.data());
    const Color cl_txt = is_edited() ? COLOR_ORANGE : color_text;
    const Align_t align = Align_t::RightTop(); // This have to be aligned this way and set up with padding, because number and units have different fonts
    padding_ui8_t extension_padding = Padding;
    if constexpr (GuiDefaults::MenuSpinHasUnits) {
        extension_padding.top = 12;
    }

    // If there is spin_off_opt::yes set in SpinConfig (with units), it prints "Off" instead of "0"
    unichar ch = spin_txt.getFirstUtf8Char();
    if (ch > 57 || (ch < 48 && ch != '-' && ch != '+')) { // first character is not a number (or +-). This is necessary because "Off" is translated
        uint16_t curr_width = extension_rect.Width();
        uint16_t off_opt_width = width(TheFont) * spin_txt.computeNumUtf8Chars() + extension_padding.left + extension_padding.right;
        if (curr_width < off_opt_width) {
            extension_rect -= Rect16::Left_t(off_opt_width - curr_width);
            extension_rect = Rect16::Width_t(off_opt_width);
        }
        render_text_align(extension_rect, spin_txt, TheFont, color_back, cl_txt, extension_padding, align); // render spin number
        return;
    }

    const Rect16 spin_rc = getSpinRect(extension_rect);
    const Rect16 unit_rc = getUnitRect(extension_rect);
    render_text_align(spin_rc, spin_txt, TheFont, color_back, cl_txt, extension_padding, align); // render spin number

    if (show_units) {
        const string_view_utf8 un = config_.unit_str();
        const unichar Utf8Char = un.getFirstUtf8Char();
        padding_ui8_t unit_padding = extension_padding;
        unit_padding.left = Utf8Char == 0xB0 ? 0 : unit__half_space_padding;
        render_text_align(unit_rc, un, TheFont, color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER, unit_padding, align); // render unit
    }
}

Rect16::Width_t WiSpin::calculateExtensionWidth(const NumericInputConfig &config) {
    const string_view_utf8 unit = config.unit_str();
    size_t unit_len = show_units ? unit.computeNumUtf8Chars() : 0;
    size_t ret = config.max_value_strlen() * width(TheFont);
    uint8_t half_space = 0;

    if (unit_len) {
        if (GuiDefaults::MenuUseFixedUnitWidth) {
            return GuiDefaults::MenuUseFixedUnitWidth;
        }
        ret += unit_len * width(GuiDefaults::FontMenuSpecial);
        ret += GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        const unichar uchar = unit.getFirstUtf8Char();
        half_space = uchar == '\xB0' ? 0 : unit__half_space_padding;
    }

    if (config.special_value.has_value()) {
        ret = std::max<size_t>(ret, _(config.special_value_str).computeNumUtf8Chars() * width(TheFont));
    }

    ret += Padding.left + Padding.right + half_space;
    return ret;
}

invalidate_t WiSpin::change(int dif) {
    const auto previous_value = value_;
    value_ = round(value_ / config_.step + dif) * config_.step;
    value_ = config_.clamp(value_, dif);

    if (!dif || value_ != previous_value) { // 0 dif forces redraw
        update();
        return invalidate_t::yes;
    } else {
        return invalidate_t::no;
    }
}

void WiSpin::update() {
    if (value_ == config_.special_value) {
        _(config_.special_value_str).copyToRAM(text_buffer_.data(), text_buffer_.size());
    } else {
        StringBuilder(text_buffer_).append_float(value_, { .max_decimal_places = config_.max_decimal_places, .all_decimal_places = true });
    }
}
