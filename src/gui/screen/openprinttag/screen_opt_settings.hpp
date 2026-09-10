/// @file
#pragma once

#include <ScreenFactory.hpp>
#include <i_window_menu_item.hpp>
#include <gui/menu_item/menu_item_cyphal_node_status.hpp>

namespace buddy::openprinttag {

/// Menu item that opens an OpenPrintTag settings screen
class MI_OPT_SETTINGS final : public IWindowMenuItem {
public:
    MI_OPT_SETTINGS();
    void click(IWindowMenu &) override;
};

class MI_OPT_READER_STATUS final : public MenuItemCyphalNodeStatus {
public:
    MI_OPT_READER_STATUS();
};

}; // namespace buddy::openprinttag
