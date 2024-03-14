/**
 * @file i_window_menu_item.cpp
 */

#include "i_window_menu_item.hpp"
#include "cmath_ext.h"
#include "gui_invalidate.hpp"
#include "img_resources.hpp"

static IWindowMenuItem *focused_menu_item = nullptr;
static bool focused_menu_item_edited = false;

static_assert(sizeof(IWindowMenuItem) <= sizeof(string_view_utf8) + sizeof(txtroll_t) + sizeof(font_t) + sizeof(int), "error inefficient size of IWindowMenuItem");

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands)
    : IWindowMenuItem(label, expands == expands_t::yes ? expand_icon_width : Rect16::Width_t(0), id_icon, enabled, hidden) {
}

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : label(label)
    , hidden((uint8_t)hidden)
    , enabled(enabled)
    , show_disabled_extension(show_disabled_extension_t::yes)
    , extension_width(extension_width_)
    , id_icon(id_icon) {
}

IWindowMenuItem::~IWindowMenuItem() {
    if (focused_menu_item == this) {
        focused_menu_item = nullptr;
        focused_menu_item_edited = false;
    }
}

bool IWindowMenuItem::is_edited() const {
    return is_focused() && focused_menu_item_edited;
}

bool IWindowMenuItem::set_is_edited(bool set) {
    if (set == is_edited()) {
        return true;
    }

    if (set && !IsEnabled()) {
        return false;
    }

    // If there is an other item currently being edited, we have to exit the edit mode
    if (focused_menu_item_edited && !focused_menu_item->try_exit_edit_mode()) {
        return false;
    }

    // Trying to edit an item -> must also set focus
    if (set) {
        set_is_focused(true);
    }

    focused_menu_item_edited = set;

    // Redraw the extension, which will probably change colour or something when edit mode changes
    InValidateExtension();

    return true;
}

IWindowMenuItem *IWindowMenuItem::edited_item() {
    return focused_menu_item_edited ? focused_menu_item : nullptr;
}

bool IWindowMenuItem::is_focused() const {
    return focused_menu_item == this;
}

bool IWindowMenuItem::set_is_focused(bool set) {
    if (set == is_focused()) {
        return true;
    }

    return move_focus(set ? this : nullptr);
}

bool IWindowMenuItem::move_focus(IWindowMenuItem *target) {
    // Changing focus - we have to cancel edit mode for previously edited item
    if (focused_menu_item_edited && !focused_menu_item->set_is_edited(false)) {
        return false;
    }

    // Redraw previously focused menu item
    if (auto *i = focused_menu_item) {
        i->roll.Stop();
        i->Invalidate();
    }

    focused_menu_item = target;

    if (auto *i = focused_menu_item) {
        if (i->IsHidden()) {
            i->show();
        }

        i->Invalidate();
        i->roll.Deinit();
    }

    return true;
}

IWindowMenuItem *IWindowMenuItem::focused_item() {
    return focused_menu_item;
}

void IWindowMenuItem::setLabelFont(font_t *src) {
    label_font = src;
}

font_t *IWindowMenuItem::getLabelFont() const {
    return label_font;
}

/*****************************************************************************/
// rectangles

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

bool IWindowMenuItem::is_touch_in_extension_rect(IWindowMenu &window_menu, point_ui16_t relative_touch_point) const {
    Rect16::Width_t width = window_menu.GetRect().Width();

    // Ensure there's enough touch area so that the value is easily touchable
    return relative_touch_point.x >= (width - std::max<int>(extension_width, minimum_touch_extension_area_width))
        && relative_touch_point.x <= width;
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
        // render_rounded_rect(getIconRect(rect), GuiDefaults::MenuColorBack, mi_color_back, GuiDefaults::MenuItemCornerRadius, MIC_TOP_LEFT | MIC_BOT_LEFT);
        printIcon(getIconRect(rect), raster_op, mi_color_back);
    }

    if (IsLabelInvalid()) {
        roll.RenderTextAlign(getLabelRect(rect), GetLabel(), getLabelFont(), mi_color_back, mi_color_text, GuiDefaults::MenuPaddingItems, GuiDefaults::MenuAlignment());
    }

    if (IsExtensionInvalid() && extension_width && icon_position != IconPosition::replaces_extends && (IsEnabled() || DoesShowDisabledExtension())) {
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

void IWindowMenuItem::printExtension(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, ropfn raster_op) const {
    render_icon_align(extension_rect, &img::arrow_right_10x16, color_back, icon_flags(Align_t::Center(), raster_op));
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

void IWindowMenuItem::touch(IWindowMenu &window_menu, [[maybe_unused]] point_ui16_t relative_touch_point) {
    click(window_menu);
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
    if (!label.is_same_ref(text)) {
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
