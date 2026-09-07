/// @file
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include <gui/menu_item/menu_item_select_menu.hpp>

/**
 * @brief Selector of footer items, with label and item index in constructor.
 */
class I_MI_FOOTER : public MenuItemSelectMenu {

public:
    I_MI_FOOTER(int item);

    int item_count() const final;
    string_view_utf8 build_item_text(int index, ItemTextParams &params) const final;

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;

private:
    const int item_;
    StringViewUtf8Parameters<4> label_params_;
};

template <size_t N>
using MI_FOOTER = WithConstructorArgs<I_MI_FOOTER, N>;

using ScreenMenuFooterSettings__ = ScreenMenu<EFooter::On, MI_RETURN, MI_FOOTER<0>
#if FOOTER_ITEMS_PER_LINE__ > 1
    ,
    MI_FOOTER<1>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    ,
    MI_FOOTER<2>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    ,
    MI_FOOTER<3>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    ,
    MI_FOOTER<4>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 5
    #error "Add more MI_FOOTER<>"
#endif
    ,
    MI_FOOTER_RESET>;

class ScreenMenuFooterSettings : public ScreenMenuFooterSettings__ {
public:
    constexpr static const char *label = N_("FOOTER");
    ScreenMenuFooterSettings();
};
