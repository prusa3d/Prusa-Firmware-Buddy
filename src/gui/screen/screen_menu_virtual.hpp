/// @file
#pragma once

#include <window_menu_virtual.hpp>
#include <screen_menu.hpp>
#include <guiconfig/GuiDefaults.hpp>

namespace screen_menu_virtual {

struct Configuration {
    /// Number of the items of the menu
    int (*item_count)();

    /// Function that constructs nth item in the menu
    void (*item_constructor)(WindowMenuVirtual::ItemVariant &variant, int index);

    /// Screen title, will be translated
    const char *title;

    EFooter footer : 1 = GuiDefaults::MenuFooter;
};

class Menu final : public WindowMenuVirtual {

public:
    Menu(window_t *parent, Rect16 rect)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {
    }

    void set_configuration(const Configuration *configuration);

    int item_count() const override;

protected:
    void setup_item(ItemVariant &variant, int index) override;

private:
    const Configuration *configuration_ = nullptr;
};

class Screen final : public ScreenMenuBase<Menu> {

public:
    Screen(const Configuration *configuration);
};

} // namespace screen_menu_virtual

using ScreenMenuVirtual = screen_menu_virtual::Screen;
