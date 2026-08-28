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
    Screens::Access()->close_screen_or_dialog();
}

MI_EXIT::MI_EXIT()
    : IWindowMenuItem(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EXIT::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->close_screen_or_dialog();
}

WI_ICON_SWITCH_OFF_ON_t::WI_ICON_SWITCH_OFF_ON_t(bool value, const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWindowMenuItem(label, 36, id_icon, enabled, hidden)
    , value_(value) //
{
    touch_extension_only_ = true;
}

void WI_ICON_SWITCH_OFF_ON_t::set_value(bool set) {
    if (value_ != set) {
        value_ = set;
        InValidateExtension();
    }
}

void WI_ICON_SWITCH_OFF_ON_t::set_invert_value(bool set) {
    if (invert_value_ != set) {
        invert_value_ = set;
        InValidateExtension();
    }
}

void WI_ICON_SWITCH_OFF_ON_t::click(IWindowMenu &) {
    value_ = !value_;
    OnChange(!value_);
    InValidateExtension();
}

void WI_ICON_SWITCH_OFF_ON_t::printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, ropfn raster_op) const {
    render_icon_align(extension_rect, (value_ ^ invert_value_) ? &img::switch_on_36x18 : &img::switch_off_36x18, color_back, Align_t::Center(), raster_op);
}
