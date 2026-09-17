/// @file
#include "dialog_menu_virtual.hpp"

#include <ScreenHandler.hpp>
#include <IDialog.hpp>
#include <window_header.hpp>
#include <window_menu_adv.hpp>
#include <WindowMenuItems.hpp>
#include <img_resources.hpp>

namespace dialog_menu_virtual {

// Size-sensitive
static_assert(sizeof(MenuItem) == 56);

// This dialog gets allocated on the stack, so we want to have the WindowMenuVirtual as small as possible
// This means ditching the default alloc buffer size for the extact MenuItem sizeof, at the cost of extra flash usage
class Menu final : public MenuBase {

public:
    Menu(window_t *parent, Rect16 rect, const Config &config)
        : WindowMenuVirtualSized(parent, rect, CloseScreenReturnBehavior::yes)
        , config(config) {
        setup_items();

        if (config.focused_item >= 0) {
            move_focus_to_index(config.focused_item);
        }
    }

    int item_count() const final {
        return config.item_count;
    }

    void handle_item_click(IWindowMenuItem &item) {
        result = *item_index(&item);
        Screens::Access()->close_dialog();
    }

protected:
    void setup_item(ItemVariant &variant, int index) final {
        config.setup_f(index, variant);
    }

public:
    const Config &config;
    std::optional<int> result;
};

class Dialog final : public IDialog {

public:
    Dialog(const Config &config)
        : IDialog(GuiDefaults::RectScreen)
        , header_(this, config.title)
        , menu_(this, GuiDefaults::RectScreenNoHeader, config) //
    {
        CaptureNormalWindow(menu_);
    }

    std::optional<int> result() const {
        return menu_.menu.result;
    }

private:
    window_header_t header_;
    WindowExtendedMenu<Menu> menu_;
};

void MenuItem::click(IWindowMenu &menu) {
    handle_click(*this, menu);
}

std::optional<int> execute(const Config &config) {
    Dialog dlg(config);
    Screens::Access()->gui_loop_until_dialog_closed();
    return dlg.result();
}

void MenuItem::handle_click(IWindowMenuItem &item, IWindowMenu &menu) {
    static_cast<Menu &>(menu).handle_item_click(item);
}

} // namespace dialog_menu_virtual
