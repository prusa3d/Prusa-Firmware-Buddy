/// @file
#pragma once

#include <gui/menu_item/menu_item_select_menu.hpp>
#include <MItem_menus.hpp>

/**
 * @brief Selector of footer items, with label and item index in constructor.
 */
class MI_FOOTER : public MenuItemSelectMenu {

public:
    MI_FOOTER(int item);

    int item_count() const final;
    string_view_utf8 build_item_text(int index, StringViewUtf8ParamBase &params) const final;

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;

private:
    const int item_;
    StringViewUtf8Parameters<4> label_params_;
};

class MI_FOOTER_SETTINGS final : public MI_SCREEN_BASE {
public:
    MI_FOOTER_SETTINGS();
};
