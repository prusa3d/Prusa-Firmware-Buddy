#pragma once

#include <gui/menu_item/menu_item_select_menu.hpp>

class MI_CURRENT_SHEET_PROFILE : public MenuItemSelectMenu {
public:
    MI_CURRENT_SHEET_PROFILE();

    int item_count() const final;
    string_view_utf8 build_item_text(int index, StringViewUtf8ParamBase &params) const final;

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;

protected:
    int item_count_ = 0;
    std::array<int, config_store_ns::sheets_num> items_;
};
