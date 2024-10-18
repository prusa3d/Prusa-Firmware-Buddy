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

/**
 * @brief Selector of footer items, with label and item index in constructor.
 */
class I_MI_FOOTER : public WI_LAMBDA_SPIN {
protected:
    /**
     * @brief Store spinner index as a selected footer type
     * @param item_n which item of a footer
     */
    void store_footer_index(size_t item_n);

    /**
     * @brief Convert spinner index to footer type.
     * @param index spinner index
     * @return footer type
     */
    static footer::Item to_footer_item(size_t index);

    /**
     * @brief Convert footer type to spinner index.
     * @param item footer type
     * @return spinner index
     */
    static size_t to_index(footer::Item item);

public:
    /**
     * @brief Construct spin footer item selector.
     * @param label label for this particular item
     * @param item_n which item of a footer
     */
    I_MI_FOOTER(const char *const label, int item_n);
};

/**
 * @brief Template for selector of footer items that provides label.
 * @param N which item of a footer
 */
template <size_t N>
class MI_FOOTER : public I_MI_FOOTER {
    virtual void OnChange() override { store_footer_index(N); } ///< Callback when spinner value changes

    static_assert(N >= 0 && N <= 4, "bad input");
    static consteval const char *get_name() {
        switch (N) {
        case 0:
            return N_("Item 1");
        case 1:
            return N_("Item 2");
        case 2:
            return N_("Item 3");
        case 3:
            return N_("Item 4");
        case 4:
            return N_("Item 5");
        }
        consteval_assert_false();
        return "";
    }

    static constexpr const char *label = get_name();

public:
    MI_FOOTER()
        : I_MI_FOOTER(label, N) {}
};

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
