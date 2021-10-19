/**
 * @file IWindowMenuItem.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuItems.hpp"
#include "resource.h"
#include "cmath_ext.h"

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands, font_t *label_font)
    : IWindowMenuItem(label, expands == expands_t::yes ? expand_icon_width : Rect16::Width_t(0), id_icon, enabled, hidden, label_font) {
}

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, font_t *label_font)
    : label(label)
    , hidden((uint8_t)hidden)
    , enabled(enabled)
    , focused(is_focused_t::no)
    , selected(is_selected_t::no)
    , id_icon(id_icon)
    , extension_width(extension_width_)
    , label_font(label_font) {
}

/*****************************************************************************/
//rectangles
Rect16 IWindowMenuItem::getCustomRect(Rect16 base_rect, uint16_t custom_rect_width) {
    Rect16 custom_rect = { base_rect.Left(), base_rect.Top(), custom_rect_width, base_rect.Height() };
    custom_rect += Rect16::Left_t(base_rect.Width() - custom_rect.Width());
    return custom_rect;
}

Rect16 IWindowMenuItem::getIconRect(Rect16 rect) const {
    rect = icon_width;
    return rect;
}

Rect16 IWindowMenuItem::getLabelRect(Rect16 rect) const {
    rect -= icon_width;
    rect -= extension_width;
    rect += Rect16::Left_t(icon_width);
    return rect;
}

Rect16 IWindowMenuItem::getExtensionRect(Rect16 rect) const {
    rect += Rect16::Left_t(rect.Width() - extension_width);
    rect = extension_width;
    return rect;
}

void IWindowMenuItem::Print(Rect16 rect) const {
    ropfn raster_op;
    raster_op.shadow = IsEnabled() ? is_shadowed::no : is_shadowed::yes;
    raster_op.swap_bw = IsFocused() ? has_swapped_bw::yes : has_swapped_bw::no;

    color_t mi_color_back = GetBackColor();
    color_t mi_color_text = GetTextColor();

    //print background
    render_rect(rect, mi_color_back);

    printIcon(getIconRect(rect), raster_op, mi_color_back);
    roll.RenderTextAlign(getLabelRect(rect), GetLabel(), getLabelFont(), mi_color_back, mi_color_text, GuiDefaults::MenuPadding, GuiDefaults::MenuAlignment());
    if (extension_width)
        printExtension(getExtensionRect(rect), mi_color_text, mi_color_back, raster_op);
}

/*  color               options: |enabled|focused|dev_only|
*   MenuColorDevelopment         | 101 or 111
*   MenuColorDevelopmentDisabled | 001 or 011
*   MenuColorBack                | 110 or 010
*   MenuColorText                | 100
*   MenuColorDisabled            | 000
*/
color_t IWindowMenuItem::GetTextColor() const {
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
    color_t ret = GuiDefaults::MenuColorBack;
    if (IsFocused()) {
        ret = IsEnabled() ? GuiDefaults::MenuColorFocusedBack : GuiDefaults::MenuColorDisabled;
    }
    return ret;
}

void IWindowMenuItem::printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const {
    //do not check id. id == 0 will render as black, it is needed
    render_icon_align(icon_rect, id_icon, color_back, icon_flags(Align_t::Center(), raster_op));
}

void IWindowMenuItem::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {
    render_icon_align(extension_rect, IDR_PNG_arrow_right_16px, color_back, icon_flags(Align_t::Center(), raster_op));
}

void IWindowMenuItem::Click(IWindowMenu &window_menu) {
    roll.Deinit();
    window_menu.Invalidate();
    if (IsEnabled()) {
        click(window_menu);
    }
}

void IWindowMenuItem::SetFocus() {
    focused = is_focused_t::yes;
    roll.Deinit();
}

void IWindowMenuItem::ClrFocus() {
    focused = is_focused_t::no;
    roll.Stop();
}

// Reinits text rolling in case of focus/defocus/click
void IWindowMenuItem::reInitRoll(Rect16 rect) {
    if (roll.NeedInit()) {
        roll.Init(rect, GetLabel(), label_font, GuiDefaults::MenuPadding, GuiDefaults::MenuAlignment());
    }
}

bool IWindowMenuItem::IsHidden() const {
    return (hidden == (uint8_t)is_hidden_t::yes) || (hidden == (uint8_t)is_hidden_t::dev && !GuiDefaults::ShowDevelopmentTools);
}
