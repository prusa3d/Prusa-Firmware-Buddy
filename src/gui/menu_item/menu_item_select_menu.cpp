#include "menu_item_select_menu.hpp"

#include <gui/ScreenHandler.hpp>
#include <window_menu_adv.hpp>
#include <window_menu_virtual.hpp>
#include <window_header.hpp>
#include <WindowMenuItems.hpp>

namespace {
class DialogItem final : public IWindowMenuItem {

public:
    DialogItem(MenuItemSelectMenu &menu, int index);

    void click(IWindowMenu &menu) override;

private:
    const int index_;
    MenuItemSelectMenu::ItemTextParams params_;
};

// Size-sensitive
static_assert(sizeof(DialogItem) == 64);

// This dialog gets allocated on the stack, so we want to have the WindowMenuVirtual as small as possible
// This means ditching the default alloc buffer size for the extact DialogItem sizeof, at the cost of extra flash usage
class DialogMenu final : public WindowMenuVirtualSized<WindowMenuVirtualBase::default_item_buffer_size, sizeof(DialogItem)> {

public:
    DialogMenu(window_t *parent, Rect16 rect, MenuItemSelectMenu &menu);

    int item_count() const final;

protected:
    void setup_item(ItemVariant &variant, int index) final;

public:
    MenuItemSelectMenu &menu;
    std::optional<int> result;
};

class Dialog final : public IDialog {

public:
    Dialog(MenuItemSelectMenu &menu);

    std::optional<int> result() const {
        return menu_.menu.result;
    }

private:
    window_header_t header_;
    WindowExtendedMenu<DialogMenu> menu_;
};

// DialogItem
// =============================================================
DialogItem::DialogItem(MenuItemSelectMenu &menu, int index)
    : index_(index) //
{
    SetLabel(menu.build_item_text(index, params_));
}

void DialogItem::click(IWindowMenu &menu) {
    static_cast<DialogMenu &>(menu).result = index_;
    Screens::Access()->Close();
}

// DialogMenu
// =============================================================
DialogMenu::DialogMenu(window_t *parent, Rect16 rect, MenuItemSelectMenu &menu)
    : WindowMenuVirtualSized(parent, rect, CloseScreenReturnBehavior::yes)
    , menu(menu) //
{
    setup_items();
}

int DialogMenu::item_count() const {
    return menu.item_count() + 1;
}

void DialogMenu::setup_item(ItemVariant &variant, int index) {
    if (index == 0) {
        variant.emplace<MI_RETURN>();
    } else {
        variant.emplace<DialogItem>(menu, index - 1);
    }
}

// Dialog
// =============================================================
Dialog::Dialog(MenuItemSelectMenu &menu)
    : IDialog(GuiDefaults::RectScreen)
    , header_(this, menu.GetLabel())
    , menu_(this, GuiDefaults::RectScreenNoHeader, menu) //
{
    CaptureNormalWindow(menu_);
    menu_.menu.move_focus_to_index(menu.current_item() + 1);
}

} // namespace

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

    int new_item;

    switch (behavior_) {

    case Behavior::select_only:
    case Behavior::submenu: {

        {
            // The dialog is quite big - keep it on stack as shortly as possible
            Dialog dlg(*this);
            Screens::Access()->gui_loop_until_dialog_closed();
            new_item = dlg.result().value_or(current_item_);
        }

        // Opening a dialog with a menu screws up focus for the current menu - we need to restore it
        menu.move_focus_to_index(prev_focus);
        break;
    }

    case Behavior::quick_cycle:
        new_item = (current_item_ + 1) % item_count();
        break;
    }

    if (new_item == current_item_) {
        return;
    }

    const OnItemSelectedArgs args {
        .old_index = current_item_,
        .new_index = new_item,
        .menu = menu,
    };
    if (!on_item_selected(args)) {
        return;
    }

    set_current_item(new_item);
}
