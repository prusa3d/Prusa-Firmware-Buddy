/**
 * @file screen_menu_footer_settings.hpp
 * @brief settings of menu footer items
 */

#pragma once

#include "WindowItemFormatableSpin.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"
#include <gui/menu_item/menu_item_select_menu.hpp>

/**
 * @brief Selector of footer items, with label and item index in constructor.
 */
class I_MI_FOOTER : public MenuItemSelectMenu {

public:
    I_MI_FOOTER(int item);

    int item_count() const final;
    void build_item_text(int index, const std::span<char> &buffer) const final;

protected:
    bool on_item_selected(int old_index, int new_index) override;

private:
    const int item_;
    StringViewUtf8Parameters<4> label_params_;
};

template <size_t N>
using MI_FOOTER = WithConstructorArgs<I_MI_FOOTER, N>;

class MI_LEFT_ALIGN_TEMP : public MenuItemSwitch {
public:
    MI_LEFT_ALIGN_TEMP();
    virtual void OnChange(size_t /*old_index*/) override;
};

class MI_SHOW_ZERO_TEMP_TARGET : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Temp. show zero");

public:
    MI_SHOW_ZERO_TEMP_TARGET();
    virtual void OnChange(size_t old_index) override;
};

class MI_FOOTER_CENTER_N : public WiSpin {
    constexpr static const char *const label = N_("Center N and Fewer Items");

public:
    MI_FOOTER_CENTER_N();
    virtual void OnClick() override;
};

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
    MI_FOOTER_SETTINGS_ADV, MI_FOOTER_RESET>;

class ScreenMenuFooterSettings : public ScreenMenuFooterSettings__ {
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("FOOTER");
    ScreenMenuFooterSettings();
};

using ScreenMenuFooterSettingsAdv__ = ScreenMenu<EFooter::On, MI_RETURN, MI_FOOTER_CENTER_N, MI_LEFT_ALIGN_TEMP, MI_SHOW_ZERO_TEMP_TARGET>;

class ScreenMenuFooterSettingsAdv : public ScreenMenuFooterSettingsAdv__ {
public:
    constexpr static const char *label = N_("FOOTER ADVANCED");
    ScreenMenuFooterSettingsAdv();
};
