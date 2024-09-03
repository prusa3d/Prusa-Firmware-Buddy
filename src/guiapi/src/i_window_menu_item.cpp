/**
 * @file i_window_menu_item.cpp
 */

#include "i_window_menu_item.hpp"
#include "cmath_ext.h"
#include "gui_invalidate.hpp"
#include "img_resources.hpp"

#include <gui/event/focus_event.hpp>
#include <gui/event/touch_event.hpp>

namespace window_menu_item_private {

IWindowMenuItem *focused_menu_item = nullptr;
bool focused_menu_item_edited = false;
txtroll_t focused_menu_item_roll;

} // namespace window_menu_item_private

using namespace window_menu_item_private;

constexpr IWindowMenuItem::ColorScheme IWindowMenuItem::color_scheme_title = {
    .text = {
        .focused = COLOR_WHITE,
        .unfocused = COLOR_WHITE,
    },
    .back = {
        .focused = Color::from_raw(0x00AAAAAA),
        .unfocused = Color::from_raw(0x00333333),
    },
};

IWindowMenuItem::IWindowMenuItem(const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands)
    : IWindowMenuItem(label, expands == expands_t::yes ? expand_icon_width : Rect16::Width_t(0), id_icon, enabled, hidden) {
}

IWindowMenuItem::IWindowMenuItem(const string_view_utf8 &label, Rect16::Width_t extension_width_, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
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

void IWindowMenuItem::set_is_enabled(bool set) {
    if (IsEnabled() == set) {
        return;
    }

    enabled = is_enabled_t(set);
    Invalidate();
}

void IWindowMenuItem::set_show_disabled_extension(bool set_) {
    const auto set = show_disabled_extension_t(set_);

    if (show_disabled_extension == set) {
        return;
    }

    show_disabled_extension = set;
    Invalidate();
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
    // Moving focus to the same item -> instant success
    if (target == focused_menu_item) {
        return true;
    }

    // Changing focus - we have to cancel edit mode for previously edited item
    if (focused_menu_item_edited && !focused_menu_item->set_is_edited(false)) {
        return false;
    }

    IWindowMenuItem *previous_focused_item = focused_menu_item;

    // Redraw previously focused menu item
    if (previous_focused_item) {
        previous_focused_item->Invalidate();
    }

    focused_menu_item_roll.Deinit();
    focused_menu_item = target;

    if (target) {
        if (target->IsHidden()) {
            target->show();
        }

        target->Invalidate();
    }

    if (previous_focused_item) {
        // We don't know the menu, so we cannot provide it
        WindowMenuItemEventContext ctx(gui_event::FocusOutEvent {}, nullptr);
        previous_focused_item->event(ctx);
    }

    if (target) {
        // We don't know the menu, so we cannot provide it
        WindowMenuItemEventContext ctx(gui_event::FocusInEvent {}, nullptr);
        target->event(ctx);
    }

    return true;
}

IWindowMenuItem *IWindowMenuItem::focused_item() {
    return focused_menu_item;
}

void IWindowMenuItem::setLabelFont(Font src) {
    label_font = src;
}

Font IWindowMenuItem::getLabelFont() const {
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

    Color mi_color_back = GetBackColor();
    Color mi_color_text = GetTextColor();

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

    const auto label_rect = getLabelRect(rect);

    if (is_focused() && focused_menu_item_roll.NeedInit()) {
        focused_menu_item_roll.Init(label_rect, label, label_font, GuiDefaults::MenuPaddingItems, GuiDefaults::MenuAlignment());
    }

    if (IsLabelInvalid()) {
        if (is_focused()) {
            // Is focused -> use shared roll instance
            focused_menu_item_roll.render_text(label_rect, label, label_font, mi_color_back, mi_color_text, GuiDefaults::MenuPaddingItems, GuiDefaults::MenuAlignment());

        } else {
            // Not focused -> render without roll
            render_text_align(label_rect, label, label_font, mi_color_back, mi_color_text, GuiDefaults::MenuPaddingItems, GuiDefaults::MenuAlignment(), true);
        }
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
Color IWindowMenuItem::GetTextColor() const {
    if (clr_scheme) {
        return IsFocused() ? clr_scheme->text.focused : clr_scheme->text.unfocused;
    }

    Color ret;
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
Color IWindowMenuItem::GetBackColor() const {
    if (clr_scheme) {
        return IsFocused() ? clr_scheme->back.focused : clr_scheme->back.unfocused;
    }

    Color ret = GuiDefaults::MenuColorBack;
    if (IsFocused()) {
        ret = IsEnabled() ? GuiDefaults::MenuColorFocusedBack : GuiDefaults::MenuColorDisabled;
    }
    return ret;
}

void IWindowMenuItem::printIcon(Rect16 icon_rect, ropfn raster_op, Color color_back) const {
    if (id_icon) {
        render_icon_align(icon_rect, id_icon, color_back, icon_flags(Align_t::Center(), raster_op));
    }
}

void IWindowMenuItem::printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, ropfn raster_op) const {
    render_icon_align(extension_rect, &img::arrow_right_10x16, color_back, icon_flags(Align_t::Center(), raster_op));
}

void IWindowMenuItem::Click(IWindowMenu &window_menu) {
    if (IsEnabled()) {
        focused_menu_item_roll.Deinit();
        InValidateExtension();
        click(window_menu);
    }
}

void IWindowMenuItem::Touch([[maybe_unused]] IWindowMenu &window_menu, [[maybe_unused]] point_ui16_t relative_touch_point) {
#if HAS_TOUCH()
    if (IsEnabled()) {
        focused_menu_item_roll.Deinit();
        InValidateExtension();

        WindowMenuItemEventContext ctx(gui_event::TouchEvent { relative_touch_point }, &window_menu);
        event(ctx);
    }
#endif
}

bool IWindowMenuItem::IsHidden() const {
    return (hidden == (uint8_t)is_hidden_t::yes) || (hidden == (uint8_t)is_hidden_t::dev && !GuiDefaults::ShowDevelopmentTools);
}

bool IWindowMenuItem::IsDevOnly() const {
    return hidden == (uint8_t)is_hidden_t::dev && GuiDefaults::ShowDevelopmentTools;
}

void IWindowMenuItem::SetIconId(const img::Resource *id) {
    if (id_icon == id) {
        return;
    }

    id_icon = id;
    InValidateIcon();
}

void IWindowMenuItem::SetLabel(const string_view_utf8 &text) {
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
    if (clr_scheme == scheme) {
        return;
    }

    clr_scheme = scheme;
    Invalidate();
}

void IWindowMenuItem::reset_color_scheme() {
    set_color_scheme(nullptr);
}

void IWindowMenuItem::set_icon_position(const IconPosition position) {
    icon_position = position;
}

auto IWindowMenuItem::get_icon_position() const -> IconPosition {
    return icon_position;
}

void IWindowMenuItem::handle_roll() {
    if (focused_menu_item && focused_menu_item_roll.Tick() == invalidate_t::yes) {
        focused_menu_item->InValidateLabel();
    }
}

void IWindowMenuItem::reset_roll() {
    focused_menu_item_roll.Deinit();
}

bool IWindowMenuItem::Change(int dif) {
    bool changed = change(dif) == invalidate_t::yes;
    if (changed) {
        InValidateExtension();
    }
    return changed;
}

void IWindowMenuItem::event(WindowMenuItemEventContext &ctx) {
    // The event has been processed & accepted -> do nothing
    if (ctx.is_accepted()) {
        return;
    }

#if HAS_TOUCH()
    if (const auto *e = ctx.event.value_maybe<gui_event::TouchEvent>()) {
        assert(ctx.menu);
        if (!touch_extension_only_ || is_touch_in_extension_rect(*ctx.menu, e->relative_touch_point)) {
            click(*ctx.menu);
        }

        // Accept touch in every case - we don't want the event to keep propagating
        ctx.accept();
    }
#endif
}
