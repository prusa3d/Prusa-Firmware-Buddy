/**
 * @file IWindowMenuItem.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuItems.hpp"
#include "resource.h"
#include "cmath_ext.h"

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands)
    : IWindowMenuItem(label, expands == expands_t::yes ? expand_icon_width : Rect16::Width_t(0), id_icon, enabled, hidden) {
}

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : label(label)
    , hidden(hidden)
    , enabled(enabled)
    , focused(is_focused_t::no)
    , selected(is_selected_t::no)
    , id_icon(id_icon)
    , extension_width(extension_width_) {
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
    color_t color_text = IsEnabled() ? GuiDefaults::MenuColorText : GuiDefaults::MenuColorDisabled;
    color_t color_back = GuiDefaults::MenuColorBack;
    uint8_t swap = IsEnabled() ? 0 : ROPFN_DISABLE;

    if (IsFocused()) {
        SWAP(color_text, color_back);
        swap |= ROPFN_SWAPBW;
    }

    printIcon(getIconRect(rect), swap, GuiDefaults::MenuColorBack);
    printLabel(getLabelRect(rect), color_text, color_back);
    if (extension_width)
        printExtension(getExtensionRect(rect), color_text, color_back, swap);
}

void IWindowMenuItem::printIcon(Rect16 icon_rect, uint8_t swap, color_t color_back) const {
    //do not check id. id == 0 will render as black, it is needed
    render_icon_align(icon_rect, id_icon, color_back, RENDER_FLG(ALIGN_CENTER, swap));
}

void IWindowMenuItem::printLabel(Rect16 label_rect, color_t color_text, color_t color_back) const {
    roll.RenderTextAlign(label_rect, GetLabel(), GuiDefaults::FontMenuItems, color_back, color_text, GuiDefaults::MenuPadding, GuiDefaults::MenuAlignment);
}

void IWindowMenuItem::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    render_icon_align(extension_rect, IDR_PNG_arrow_right_16px, GuiDefaults::MenuColorBack, RENDER_FLG(ALIGN_LEFT_CENTER, swap));
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
    //cannot call InitRollIfNeeded(rect), rect not known (cannot add it into param)
    roll.Deinit();
}

void IWindowMenuItem::ClrFocus() {
    focused = is_focused_t::no;
    roll.Stop();
}

// Reinits text rolling in case of focus/defocus/click
void IWindowMenuItem::reInitRoll(Rect16 rect) {
    if (roll.NeedInit()) {
        roll.Init(rect, GetLabel(), GuiDefaults::FontMenuItems, GuiDefaults::MenuPadding, GuiDefaults::MenuAlignment);
    }
}
