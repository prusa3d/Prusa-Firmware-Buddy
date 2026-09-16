#include "menu_item_select_menu.hpp"

#include <gui/dialogs/dialog_menu_virtual.hpp>
#include <MItem_tools.hpp>

MenuItemSelectMenu::MenuItemSelectMenu(const string_view_utf8 &label)
    : IWindowMenuItem(label, 1) {}

void MenuItemSelectMenu::set_behavior(Behavior set) {
    behavior_ = set;

    switch (set) {

    case Behavior::select_only:
        current_item_ = -1;
        set_show_expand_icon();
        break;

    case Behavior::submenu:
    case Behavior::quick_cycle:
        // No extra changes
        break;
    }
}

void MenuItemSelectMenu::set_current_item(int set) {
    if (current_item_ != set) {
        force_set_current_item(set);
    }
}

void MenuItemSelectMenu::force_set_current_item(int set) {
    if (behavior_ == Behavior::select_only) {
        // Always keep current_item at -1
        return;
    }

    if (set < 0 || set >= item_count()) {
        return;
    }

    auto old_extension_width = extension_width;

    current_item_ = set;
    value_ = build_item_text(set, value_params_);
    extension_width = resource_font(value_font)->w * (value_.computeNumUtf8Chars() + (GuiDefaults::MenuSwitchHasBrackets ? 2 : 0));

    // When we do only InValidateExtension(), we only 'delete' text with new extension_width
    // when new width is shorter, we leave part of the old text on screen, so that is why Invalidate()
    // is needed to redraw the whole element
    if (old_extension_width > extension_width) {
        Invalidate();
    } else {
        InValidateExtension();
    }
}

Color MenuItemSelectMenu::resolved_value_text_color(Color base_color) const {
    return (is_focused() && IsEnabled()) ? GuiDefaults::ColorSelected : base_color;
}

void MenuItemSelectMenu::printExtension(Rect16 extension_rect, Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {
    if (behavior_ == Behavior::select_only) {
        // Handles drawing "expands" icon
        IWindowMenuItem::printExtension(extension_rect, color_text, color_back, raster_op);
        return;
    }

    if (current_item_ < 0 || current_item_ >= item_count()) {
        return;
    }

    const auto font_w = resource_font(value_font)->w;

    // extension_rect = Rect16::fromLTWH(extension_rect.Left(), extension_rect.Top(), extension_rect.Width(), extension_rect.Height() - 4);

    if constexpr (GuiDefaults::MenuSwitchHasBrackets) {
        const auto bracket_color = (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER;

        // Use different brackets for different behaviors
        // This is so that the user knows what pressing the item will do before they press it
        static constexpr EnumArray<Behavior, std::array<const char *, 2>, static_cast<int>(Behavior::_last) + 1> behavior_brackes {
            { Behavior::submenu, { "[", "]" } },
            { Behavior::quick_cycle, { "<", ">" } },
            { Behavior::select_only, { "", "" } }, // Dead path
        };

        const auto rct1 = Rect16::fromLTWH(extension_rect.Left(), extension_rect.Top(), font_w, extension_rect.Height());
        render_text_align(rct1, string_view_utf8::MakeCPUFLASH(behavior_brackes[behavior_][0]), value_font, color_back, bracket_color, {}, Align_t::Center(), false);

        const auto rct2 = Rect16::fromLTWH(extension_rect.Right() - font_w, extension_rect.Top(), font_w, extension_rect.Height());
        render_text_align(rct2, string_view_utf8::MakeCPUFLASH(behavior_brackes[behavior_][1]), value_font, color_back, bracket_color, {}, Align_t::Center(), false);

        extension_rect = Rect16::fromLTRB(extension_rect.Left() + font_w, extension_rect.Top(), extension_rect.EndPoint().x - font_w, extension_rect.EndPoint().y);
    }

    render_text_align(extension_rect, value_, value_font, color_back, resolved_value_text_color(color_text), {}, Align_t::Center(), false);
}

void MenuItemSelectMenu::click(IWindowMenu &menu) {
    const auto prev_focus = menu.focused_item_index();

    std::optional<int> new_item;

    switch (behavior_) {

    case Behavior::select_only:
    case Behavior::submenu: {
        const dialog_menu_virtual::Config cfg {
            .title = GetLabel(),
            .item_count = item_count() + 1, // + return
            .focused_item = current_item_ + 1, // + return
            .setup_f = [this](int index, dialog_menu_virtual::ItemVariant &variant) {
                if (index == 0) {
                    variant.emplace<MI_RETURN>();
                } else {
                    auto &item = variant.emplace<dialog_menu_virtual::MenuItem>();
                    item.SetLabel(build_item_text(index - 1, item.params));
                }
            },
        };

        const auto result = dialog_menu_virtual::execute(cfg);
        if (result.has_value() && result > 0) {
            new_item = *result - 1; // - return
        }

        // Opening a dialog with a menu screws up focus for the current menu - we need to restore it
        menu.move_focus_to_index(prev_focus);
        break;
    }

    case Behavior::quick_cycle:
        new_item = (current_item_ + 1) % item_count();
        break;
    }

    if (!new_item.has_value()) {
        return;
    }

    if (new_item == current_item_ && deduplicate_on_item_selected_) {
        return;
    }

    const OnItemSelectedArgs args {
        .old_index = current_item_,
        .new_index = *new_item,
        .menu = menu,
    };
    if (!on_item_selected(args)) {
        return;
    }

    set_current_item(*new_item);
}
