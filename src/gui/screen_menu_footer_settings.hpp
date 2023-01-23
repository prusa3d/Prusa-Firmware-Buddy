/**
 * @file screen_menu_footer_settings.hpp
 * @brief settings of menu footer items
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "menu_items_open.hpp"

class IMiFooter : public WI_SWITCH_t<size_t(footer::items::count_) + 1> {
public:
    IMiFooter(size_t index_);
};

template <size_t INDEX>
class MiFooter : public IMiFooter {
public:
    MiFooter()
        : IMiFooter(INDEX) {}

    virtual void OnChange(size_t old_index) override {
        if (index > size_t(footer::items::count_))
            return; // should not happen
        StatusFooter::SetSlotInit(INDEX, footer::items(index));
    }
};

class MI_LEFT_ALIGN_TEMP : public WI_SWITCH_t<3> {
    constexpr static const char *const label = N_("Temp. style");
    constexpr static const char *str_0 = "Static";
    constexpr static const char *str_1 = "Static-left";
    constexpr static const char *str_2 = "Dynamic";

public:
    MI_LEFT_ALIGN_TEMP();
    virtual void OnChange(size_t /*old_index*/) override;
};

class MI_SHOW_ZERO_TEMP_TARGET : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Temp. show zero");

public:
    MI_SHOW_ZERO_TEMP_TARGET();
    virtual void OnChange(size_t old_index) override;
};

class MI_FOOTER_CENTER_N : public WiSpinInt {
    constexpr static const char *const label = N_("Center N and fewer items");

public:
    MI_FOOTER_CENTER_N();
    virtual void OnClick() override;
};

using ScreenMenuFooterSettings__ = ScreenMenu<EFooter::On, MI_RETURN, MiFooter<0>
#if FOOTER_ITEMS_PER_LINE__ > 1
    ,
    MiFooter<1>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    ,
    MiFooter<2>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    ,
    MiFooter<3>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    ,
    MiFooter<4>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 5
    ,
    MiFooter<5>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 6
    ,
    MiFooter<6>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 7
    ,
    MiFooter<7>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 8
    ,
    MiFooter<8>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 9
    ,
    MiFooter<9>
#endif
    ,
    MI_FOOTER_SETTINGS_ADV, MI_FOOTER_RESET>;

class ScreenMenuFooterSettings : public ScreenMenuFooterSettings__ {
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

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
