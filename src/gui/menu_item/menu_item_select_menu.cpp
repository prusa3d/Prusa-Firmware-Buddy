#include "menu_item_select_menu.hpp"

#include <gui/ScreenHandler.hpp>
#include <window_menu_adv.hpp>

namespace {
class DialogItem final : public IWindowMenuItem {

public:
    DialogItem(MenuItemSelectMenu &menu, int index);

    void click(IWindowMenu &menu) override;

private:
    const int index_;
    std::array<char, MenuItemSelectMenu::value_buffer_size> label_ { '\0' };
};

class DialogMenu final : public WindowMenuVirtual<MI_RETURN, DialogItem> {

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
    : IWindowMenuItem({})
    , index_(index) //
{
    menu.build_item_text(index, label_);
    SetLabel(string_view_utf8::MakeRAM(label_.data()));
}

void DialogItem::click(IWindowMenu &menu) {
    static_cast<DialogMenu &>(menu).result = index_;
    Screens::Access()->Close();
}

// DialogMenu
// =============================================================
DialogMenu::DialogMenu(window_t *parent, Rect16 rect, MenuItemSelectMenu &menu)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes)
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

void MenuItemSelectMenu::set_current_item(int set) {
    if (current_item_ != set) {
        force_set_current_item(set);
    }
}

void MenuItemSelectMenu::force_set_current_item(int set) {
    if (set < 0 || set >= item_count()) {
        return;
    }

    current_item_ = set;
    build_item_text(set, value_text_);
    extension_width = resource_font(value_font)->w * (strlen(value_text_.data()) + (GuiDefaults::MenuSwitchHasBrackets ? 2 : 0));
    InValidateExtension();
}

void MenuItemSelectMenu::printExtension(Rect16 extension_rect, Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {
    if (current_item_ < 0 || current_item_ >= item_count()) {
        return;
    }

    const auto font_w = resource_font(value_font)->w;

    // extension_rect = Rect16::fromLTWH(extension_rect.Left(), extension_rect.Top(), extension_rect.Width(), extension_rect.Height() - 4);

    if constexpr (GuiDefaults::MenuSwitchHasBrackets) {
        const auto bracket_color = (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER;

        const auto rct1 = Rect16::fromLTWH(extension_rect.Left(), extension_rect.Top(), font_w, extension_rect.Height());
        render_text_align(rct1, string_view_utf8::MakeCPUFLASH("["), value_font, color_back, bracket_color, {}, Align_t::Center(), false);

        const auto rct2 = Rect16::fromLTWH(extension_rect.Right() - font_w, extension_rect.Top(), font_w, extension_rect.Height());
        render_text_align(rct2, string_view_utf8::MakeCPUFLASH("]"), value_font, color_back, bracket_color, {}, Align_t::Center(), false);

        extension_rect = Rect16::fromLTRB(extension_rect.Left() + font_w, extension_rect.Top(), extension_rect.EndPoint().x - font_w, extension_rect.EndPoint().y);
    }

    const auto text_color = (IsFocused() && IsEnabled()) ? GuiDefaults::ColorSelected : color_text;
    render_text_align(extension_rect, string_view_utf8::MakeRAM(value_text_.data()), value_font, color_back, text_color, {}, Align_t::Center(), false);
}

void MenuItemSelectMenu::click(IWindowMenu &menu) {
    const auto prev_focus = menu.focused_item_index();

    int new_item;
    {
        // The dialog is quite big - keep it on stack as shortly as possible
        Dialog dlg(*this);
        Screens::Access()->gui_loop_until_dialog_closed();
        new_item = dlg.result().value_or(current_item_);
    }

    // Opening a dialog with a menu screws up focus for the current menu - we need to restore it
    menu.move_focus_to_index(prev_focus);

    if (new_item == current_item_) {
        return;
    }

    if (!on_item_selected(current_item_, new_item)) {
        return;
    }

    set_current_item(new_item);
}
