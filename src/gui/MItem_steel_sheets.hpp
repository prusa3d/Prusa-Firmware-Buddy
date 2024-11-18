#pragma once

#include <gui/menu_item/menu_item_select_menu.hpp>

class MI_CURRENT_SHEET_PROFILE : public MenuItemSelectMenu {
public:
    MI_CURRENT_SHEET_PROFILE();

    int item_count() const final;
    void build_item_text(int index, const std::span<char> &buffer) const final;

protected:
    bool on_item_selected(int old_index, int new_index) override;

protected:
    int item_count_ = 0;
    std::array<int, config_store_ns::sheets_num> items_;
};
