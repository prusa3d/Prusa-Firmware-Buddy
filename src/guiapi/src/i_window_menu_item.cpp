/**
 * @file i_window_menu_item.cpp
 */

#include "i_window_menu_item.hpp"
#include "cmath_ext.h"
#include "gui_invalidate.hpp"
#include "png_resources.hpp"

static_assert(sizeof(IWindowMenuItem) <= sizeof(string_view_utf8) + sizeof(txtroll_t) + sizeof(font_t) + sizeof(int), "error inefficient size of IWindowMenuItem");

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands)
    : IWindowMenuItem(label, expands == expands_t::yes ? expand_icon_width : Rect16::Width_t(0), id_icon, enabled, hidden) {
}

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : label(label)
    , hidden((uint8_t)hidden)
    , enabled(enabled)
    , extension_width(extension_width_)
    , id_icon(id_icon) {
}

void IWindowMenuItem::setLabelFont(font_t *src) {
    label_font = src;
}

font_t *IWindowMenuItem::getLabelFont() const {
    return label_font;
}

/*****************************************************************************/
//rectangles

Rect16 IWindowMenuItem::getIconRect(Rect16 rect) const {
    if (icon_position == IconPosition::right) {
        rect = Rect16::Left_t { static_cast<int16_t>(rect.EndPoint().x - (icon_width * 3 - icon_width / 2)) };
    } else if (icon_position == IconPosition::replaces_extends) {
        rect = Rect16::Left_t { static_cast<int16_t>(rect.EndPoint().x - (icon_width * 2 - icon_width / 2)) };
    }
    rect = icon_width;
    return rect;
}

Rect16 IWindowMenuItem::getLabelRect(Rect16 rect) const {
    if (icon_position == IconPosition::right) {
        rect -= Rect16::Width_t { icon_width * 4 - icon_width / 2 };
    } else if (icon_position == IconPosition::replaces_extends) {
        rect -= Rect16::Width_t { icon_width * 3 - icon_width / 2 };
    } else {
        rect -= icon_width;
    }

    if (icon_position != IconPosition::replaces_extends) {
        rect -= Rect16::Width_t(extension_width);
    }
    rect += Rect16::Left_t(icon_width);
    return rect;
}

Rect16 IWindowMenuItem::getExtensionRect(Rect16 rect) const {
    rect += Rect16::Left_t(rect.Width() - extension_width);
    rect = Rect16::Width_t(extension_width);
    return rect;
}

void IWindowMenuItem::Print(Rect16 rect) {
    ropfn raster_op;
    if (clr_scheme) {
        raster_op = IsFocused() ? clr_scheme->rop.focused : clr_scheme->rop.unfocused;
    } else {
        raster_op.shadow = IsEnabled() ? is_shadowed::no : is_shadowed::yes;
        raster_op.swap_bw = IsFocused() ? has_swapped_bw::yes : has_swapped_bw::no;
    }

    color_t mi_color_back = GetBackColor();
    color_t mi_color_text = GetTextColor();

    if (IsIconInvalid() && IsLabelInvalid() && IsExtensionInvalid()) {
        render_rounded_rect(rect, GuiDefaults::MenuColorBack, mi_color_back, GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS);
    }

    // Adjust menu item rectangle (simple padding on the sides)
    rect += Rect16::Left_t(GuiDefaults::MenuItemCornerRadius);
    rect -= Rect16::Width_t(2 * GuiDefaults::MenuItemCornerRadius);

    if (IsIconInvalid()) {
        // Unnecessary invalidation of bg - use commented code if reprinting causes drawing artefacts
        //render_rounded_rect(getIconRect(rect), GuiDefaults::MenuColorBack, mi_color_back, GuiDefaults::MenuItemCornerRadius, MIC_TOP_LEFT | MIC_BOT_LEFT);
        printIcon(getIconRect(rect), raster_op, mi_color_back);
    }

    if (IsLabelInvalid()) {
        roll.RenderTextAlign(getLabelRect(rect), GetLabel(), getLabelFont(), mi_color_back, mi_color_text, GuiDefaults::MenuPaddingItems, GuiDefaults::MenuAlignment());
    }

    if (IsExtensionInvalid() && extension_width && icon_position != IconPosition::replaces_extends) {
        render_rect(getExtensionRect(rect), mi_color_back);
        printExtension(getExtensionRect(rect), mi_color_text, mi_color_back, raster_op);
    }

    Validate();
}

/*  color               options: |enabled|focused|dev_only|
*   MenuColorDevelopment         | 101 or 111
*   MenuColorDevelopmentDisabled | 001 or 011
*   MenuColorBack                | 110 or 010
*   MenuColorText                | 100
*   MenuColorDisabled            | 000
*/
color_t IWindowMenuItem::GetTextColor() const {
    if (clr_scheme) {
        return IsFocused() ? clr_scheme->text.focused : clr_scheme->text.unfocused;
    }

    color_t ret;
    if (IsEnabled() && hidden == (uint8_t)is_hidden_t::dev) {
        ret = GuiDefaults::MenuColorDevelopment;
    } else if (hidden == (uint8_t)is_hidden_t::dev) {
        ret = GuiDefaults::MenuColorDevelopmentDisabled;
    } else if (IsFocused()) {
        ret = GuiDefaults::MenuColorBack;
    } else if (IsEnabled()) {
        ret = GuiDefaults::MenuColorText;
    } else {
        ret = GuiDefaults::MenuColorDisabled;
    }
    return ret;
}

/*  color               options: |enabled|focused|
*   MenuColorBack                | 10 or 00
*   MenuColorFocusedBack         | 11
*   MenuColorDisabled            | 01
*/
color_t IWindowMenuItem::GetBackColor() const {
    if (clr_scheme) {
        return IsFocused() ? clr_scheme->back.focused : clr_scheme->back.unfocused;
    }

    color_t ret = GuiDefaults::MenuColorBack;
    if (IsFocused()) {
        ret = IsEnabled() ? GuiDefaults::MenuColorFocusedBack : GuiDefaults::MenuColorDisabled;
    }
    return ret;
}

void IWindowMenuItem::printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const {
    if (id_icon) {
        render_icon_align(icon_rect, id_icon, color_back, icon_flags(Align_t::Center(), raster_op));
    }
}

void IWindowMenuItem::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {
    render_icon_align(extension_rect, &png::arrow_right_10x16, color_back, icon_flags(Align_t::Center(), raster_op));
}

void IWindowMenuItem::Click(IWindowMenu &window_menu) {
    if (IsEnabled()) {
        roll.Deinit();
        InValidateExtension();
        click(window_menu);
    }
}

void IWindowMenuItem::Touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    if (IsEnabled()) {
        roll.Deinit();
        InValidateExtension();
        touch(window_menu, relative_touch_point);
    }
}

void IWindowMenuItem::touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    click(window_menu);
}

void IWindowMenuItem::setFocus() {
    if (IsHidden()) {
        show();
    }
    focused = is_focused_t::yes;
    roll.Deinit();
    Invalidate();
}

void IWindowMenuItem::clrFocus() {
    focused = is_focused_t::no;
    roll.Stop();
    Invalidate();
}

// Reinits text rolling in case of focus/defocus/click
void IWindowMenuItem::reInitRoll(Rect16 rect) {
    if (roll.NeedInit()) {
        roll.Init(rect, GetLabel(), label_font, GuiDefaults::MenuPaddingItems, GuiDefaults::MenuAlignment());
    }
}

void IWindowMenuItem::deInitRoll() {
    roll.Deinit();
}

bool IWindowMenuItem::IsHidden() const {
    return (hidden == (uint8_t)is_hidden_t::yes) || (hidden == (uint8_t)is_hidden_t::dev && !GuiDefaults::ShowDevelopmentTools);
}

bool IWindowMenuItem::IsDevOnly() const {
    return hidden == (uint8_t)is_hidden_t::dev && GuiDefaults::ShowDevelopmentTools;
}

void IWindowMenuItem::SetLabel(string_view_utf8 text) {
    if (label != text) {
        label = text;
        InValidateLabel();
    }
}

bool IWindowMenuItem::IsInvalid() const {
    // order matters label is most likely to be invalid and icon least likely
    // so this order is the most efficient
    return IsLabelInvalid() || IsExtensionInvalid() || IsIconInvalid();
}

bool IWindowMenuItem::IsIconInvalid() const {
    return invalid_icon;
}

bool IWindowMenuItem::IsLabelInvalid() const {
    return invalid_label;
}

bool IWindowMenuItem::IsExtensionInvalid() const {
    return invalid_extension;
}

void IWindowMenuItem::Validate() {
    invalid_icon = false;
    invalid_label = false;
    invalid_extension = false;
}

void IWindowMenuItem::Invalidate() {
    invalid_icon = true;
    invalid_label = true;
    invalid_extension = true;
    gui_invalidate();
}

void IWindowMenuItem::InValidateIcon() {
    invalid_icon = true;
    gui_invalidate();
}

void IWindowMenuItem::InValidateLabel() {
    invalid_label = true;
    gui_invalidate();
}

void IWindowMenuItem::InValidateExtension() {
    invalid_extension = true;
    gui_invalidate();
}

void IWindowMenuItem::set_color_scheme(const ColorScheme *scheme) {
    clr_scheme = scheme;
}

void IWindowMenuItem::reset_color_scheme() {
    clr_scheme = nullptr;
}

void IWindowMenuItem::set_icon_position(const IconPosition position) {
    icon_position = position;
}

auto IWindowMenuItem::get_icon_position() const -> IconPosition {
    return icon_position;
}

void IWindowMenuItem::Roll() {
    if (roll.Tick() == invalidate_t::yes) {
        InValidateLabel();
    }
}

bool IWindowMenuItem::Change(int dif) {
    bool changed = change(dif) == invalidate_t::yes;
    if (changed) {
        InValidateExtension();
    }
    return changed;
}
