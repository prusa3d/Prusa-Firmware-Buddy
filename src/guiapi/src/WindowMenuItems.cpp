/**
 * @file WindowMenuItems.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuItems.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"

MI_RETURN::MI_RETURN()
    : IWindowMenuItem(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
    has_return_behavior_ = true;
}

void MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->Close();
}

MI_EXIT::MI_EXIT()
    : IWindowMenuItem(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EXIT::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->Close();
}

MI_TEST_DISABLED_RETURN::MI_TEST_DISABLED_RETURN()
    // just for test (in debug), do not translate
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH((uint8_t *)label), &img::folder_up_16x16, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_DISABLED_RETURN::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Close();
}

WI_ICON_SWITCH_OFF_ON_t::WI_ICON_SWITCH_OFF_ON_t(bool value, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWindowMenuItem(label, 36, id_icon, enabled, hidden)
    , index(value_)
    , value_(value) //
{}

void WI_ICON_SWITCH_OFF_ON_t::set_value(bool set, bool emit_change) {
    if (value_ == set) {
        return;
    }

    value_ = set;
    if (emit_change) {
        OnChange(!value_);
    }
    InValidateExtension();
}

invalidate_t WI_ICON_SWITCH_OFF_ON_t::change(int) {
    value_ = !value_;
    return invalidate_t::yes;
}

void WI_ICON_SWITCH_OFF_ON_t::touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    if (is_touch_in_extension_rect(window_menu, relative_touch_point)) {
        click(window_menu);
    }
}

void WI_ICON_SWITCH_OFF_ON_t::click(IWindowMenu &) {
    set_value(!value_, true);
}

void WI_ICON_SWITCH_OFF_ON_t::printExtension(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, ropfn raster_op) const {
    render_icon_align(extension_rect, value_ ? &img::switch_on_36x18 : &img::switch_off_36x18, color_back, { Align_t::Center(), raster_op });
}
