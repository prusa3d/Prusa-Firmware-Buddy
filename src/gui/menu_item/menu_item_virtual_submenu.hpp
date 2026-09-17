/// @file
#pragma once

#include <functional>

#include <i_window_menu.hpp>
#include <gui/screen/screen_menu_virtual.hpp>
#include <WindowMenuItems.hpp>
#include <ScreenHandler.hpp>

/// A menu item that opens ScreenMenuVirtual with item_count number of Items (and MI_RETURN)
/// Each item gets constructed with the index passed on
template <auto title, typename Item, int item_count, auto index_map_f = std::identity {}>
class MenuItemVirtualSubmenu : public IWindowMenuItem {

public:
    MenuItemVirtualSubmenu()
        : IWindowMenuItem(_(title)) {
        set_show_expand_icon();
    }

    void click(IWindowMenu &) override {
        Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenMenuVirtual>(&menu_config));
    }

protected:
    static constexpr screen_menu_virtual::Configuration menu_config {
        .item_count = [] -> int {
            return item_count + 1; // + MI_RETURN
        },
        .item_constructor = [](WindowMenuVirtual::ItemVariant &variant, int index) {
            if (index == 0) {
                variant.emplace<MI_RETURN>();
            } else {
                variant.emplace<Item>(index_map_f(index - 1));
            }
            //
        },
        .title = title,
    };
};
