/**
 * @file i_window_menu_item.cpp
 */

#include "i_window_menu_item.hpp"
#include "cmath_ext.h"
#include "gui_invalidate.hpp"
#include "img_resources.hpp"

#include <gui/event/focus_event.hpp>
#include <gui/event/touch_event.hpp>
#include <option/development_items.h>
#include <bsod/bsod.h>

namespace window_menu_item_private {

IWindowMenuItem *focused_menu_item = nullptr;
bool focused_menu_item_edited = false;
txtroll_t focused_menu_item_roll;

} // namespace window_menu_item_private

using namespace window_menu_item_private;

constexpr const IWindowMenuItem::ColorScheme IWindowMenuItem::color_scheme_default {
    .text {
        .focused = GuiDefaults::MenuColorBack,
        .unfocused = GuiDefaults::MenuColorText,
    },
    .back {
        .focused = GuiDefaults::MenuColorFocusedBack,
        .unfocused = GuiDefaults::MenuColorBack,
    },
};
constexpr const IWindowMenuItem::ColorScheme IWindowMenuItem::color_scheme_default_disabled {
    .text {
        .focused = GuiDefaults::MenuColorBack,
        .unfocused = GuiDefaults::MenuColorDisabled,
    },
    .back {
        .focused = GuiDefaults::MenuColorDisabled,
        .unfocused = GuiDefaults::MenuColorBack,
    },
};

constexpr const IWindowMenuItem::ColorScheme IWindowMenuItem::color_scheme_dev_item {
    .text {
        .focused = GuiDefaults::MenuColorDevelopment,
        .unfocused = GuiDefaults::MenuColorDevelopment,
    },
    .back {
        .focused = GuiDefaults::MenuColorFocusedBack,
        .unfocused = GuiDefaults::MenuColorBack,
    },
};

constexpr const IWindowMenuItem::ColorScheme IWindowMenuItem::color_scheme_title = {
    .text = {
        .focused = COLOR_WHITE,
        .unfocused = COLOR_WHITE,
    },
    .back = {
        .focused = Color::from_raw(0x00AAAAAA),
        .unfocused = Color::from_raw(0x00333333),
    },
};

IWindowMenuItem::IWindowMenuItem()
    : IWindowMenuItem(string_view_utf8 {}) {
}

IWindowMenuItem::IWindowMenuItem(const string_view_utf8 &label)
    : IWindowMenuItem(label, Rect16::Width_t(0), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

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

void IWindowMenuItem::set_enabled(bool set) {
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
    auto result = Rect16::fromLTWH(rect.Left(), rect.Top(), icon_width, rect.Height());

    switch (icon_position) {

    case IconPosition::left:
        break;

    case IconPosition::before_extension:
        result = Rect16::Left_t(rect.EndPoint().x - icon_width - extension_width - icon_extension_spacing);
        break;

    case IconPosition::after_extension:
        result = Rect16::Left_t(rect.EndPoint().x - icon_width);
        break;
    }
    return result;
}

Rect16 IWindowMenuItem::getLabelRect(Rect16 rect) const {
    const auto bottom_right = rect.EndPoint();
    switch (icon_position) {

    case IconPosition::left:
        return Rect16::fromLTRB(rect.Left() + icon_width, rect.Top(), bottom_right.x - extension_width, bottom_right.y);

    case IconPosition::before_extension:
    case IconPosition::after_extension:
        // Offset by icon_width from the left as well, we want all labels to be aligned
        return Rect16::fromLTRB(rect.Left() + icon_width, rect.Top(), bottom_right.x - extension_width - icon_width - icon_extension_spacing, bottom_right.y);
    }
    bsod_unreachable();
}

Rect16 IWindowMenuItem::getExtensionRect(Rect16 rect) const {
    auto result = Rect16::fromLTWH(rect.EndPoint().x - extension_width, rect.Top(), extension_width, rect.Height());
    switch (icon_position) {

    case IconPosition::left:
    case IconPosition::before_extension:
        break;

    case IconPosition::after_extension:
        result -= Rect16::Left_t(icon_width + icon_extension_spacing);
        break;
    }

    return result;
}

bool IWindowMenuItem::is_touch_in_extension_rect(IWindowMenu &window_menu, point_ui16_t relative_touch_point) const {
    Rect16::Width_t width = window_menu.GetRect().Width();

    // Ensure there's enough touch area so that the value is easily touchable
    return relative_touch_point.x >= (width - std::max<int>(extension_width, minimum_touch_extension_area_width))
        && relative_touch_point.x <= width;
}

void IWindowMenuItem::Print(Rect16 rect) {
    const ColorScheme *scheme = deduce_color_scheme();
    const bool focused = IsFocused();
    const ropfn raster_op = focused ? scheme->rop.focused : scheme->rop.unfocused;
    Color mi_color_back = focused ? scheme->back.focused : scheme->back.unfocused;
    Color mi_color_text = focused ? scheme->text.focused : scheme->text.unfocused;

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
        focused_menu_item_roll.Init(label_rect, label, label_font, GuiDefaults::MenuPaddingItems);
    }

    if (IsLabelInvalid()) {
        if (is_focused()) {
            // Is focused -> use shared roll instance
            focused_menu_item_roll.render_text(label_rect, label, label_font, mi_color_back, mi_color_text, GuiDefaults::MenuPaddingItems, Align_t::LeftTop());

        } else {
            // Not focused -> render without roll
            render_text_align(label_rect, label, label_font, mi_color_back, mi_color_text, GuiDefaults::MenuPaddingItems, text_flags(Align_t::LeftTop(), is_multiline::no, check_overflow::no), true);
        }
    }

    if (IsExtensionInvalid() && extension_width && (IsEnabled() || DoesShowDisabledExtension())) {
        const auto extension_rect = getExtensionRect(rect);
        render_rect(extension_rect, mi_color_back);
        printExtension(extension_rect, mi_color_text, mi_color_back, raster_op);
    }

    Validate();
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
    return (hidden == (uint8_t)is_hidden_t::yes) || (hidden == (uint8_t)is_hidden_t::dev && !option::development_items);
}

bool IWindowMenuItem::IsDevOnly() const {
    return hidden == (uint8_t)is_hidden_t::dev && option::development_items;
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

const IWindowMenuItem::ColorScheme *IWindowMenuItem::deduce_color_scheme() const {
    if (clr_scheme) {
        return clr_scheme;

    } else if (!IsEnabled()) {
        return &color_scheme_default_disabled;

    } else if (hidden == (uint8_t)is_hidden_t::dev) {
        return &color_scheme_dev_item;

    } else {
        return &color_scheme_default;
    }
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

void IWindowMenuItem::event(WindowMenuItemEventContext &ctx) {
    // The event has been processed & accepted -> do nothing
    if (ctx.is_accepted()) {
        return;
    }

#if HAS_TOUCH()
    if (const auto *e = ctx.event.value_maybe<gui_event::TouchEvent>()) {
        debug_assert(ctx.menu);
        if (!touch_extension_only_ || is_touch_in_extension_rect(*ctx.menu, e->relative_touch_point)) {
            click(*ctx.menu);
        }

        // Accept touch in every case - we don't want the event to keep propagating
        ctx.accept();
    }
#endif
}

void IWindowMenuItem::set_is_hidden(bool set) {
    const bool was_hidden = IsHidden();
    if (set == was_hidden) {
        return;
    }

    // set_is_hidden should only be called before the items are first rendered
    // the WindowMenu is not equipped to handle items appearing and disappearing (except for SwapVisibility)
    debug_assert(invalid_extension && invalid_icon && invalid_label);

    hidden = set;

    if (!IsHidden() && was_hidden) {
        Invalidate();
    }
}
