#include "window_toggle_switch.hpp"

#include <enum_array.hpp>

WindowToggleSwitch::WindowToggleSwitch(Tristate value, const string_view_utf8 &label)
    : IWindowMenuItem(label, 36)
    , value_(value) //
{
    touch_extension_only_ = true;
}

void WindowToggleSwitch::set_value(Tristate set, bool emit_toggled) {
    if (value_ == set) {
        return;
    }

    const auto old_value = value_;
    value_ = set;
    if (emit_toggled) {
        toggled(old_value);
    }
    InValidateExtension();
}

void WindowToggleSwitch::click(IWindowMenu &) {
    set_value(!value(), true);
}

void WindowToggleSwitch::printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, ropfn raster_op) const {
    static constexpr EnumArray<Tristate::Value, const img::Resource *, 3> icon {
        { Tristate::no, &img::switch_off_36x18 },
        { Tristate::yes, &img::switch_on_36x18 },
        { Tristate::other, &img::switch_mid_36x18 },
    };
    render_icon_align(extension_rect, icon[value_.value], color_back, { Align_t::Center(), raster_op });
}
