/**
 * @file screen_menu_fw_update.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

class MI_UPDATE_LABEL : public WI_LABEL_t {
    static constexpr const char *const label = N_("FW Update");

public:
    MI_UPDATE_LABEL();

protected:
    virtual void click(IWindowMenu &window_menu) override {};
};

class MI_UPDATE : public WI_SWITCH_t<3> {
    constexpr static const char *const str_0 = N_("Off");
    constexpr static const char *const str_1 = N_("On Restart");
    constexpr static const char *const str_2 = N_("Always");

    size_t init_index() const;

public:
    MI_UPDATE();

protected:
    virtual void OnChange(size_t) override;
};

using MenuFwUpdateContainer = WinMenuContainer<MI_RETURN, MI_UPDATE_LABEL, MI_UPDATE>;

class ScreenMenuFwUpdate : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("FW UPDATE");
    static constexpr size_t helper_lines = 4;
    static constexpr ResourceId helper_font = IDR_FNT_SPECIAL;

    MenuFwUpdateContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;
    StatusFooter footer;

public:
    ScreenMenuFwUpdate();

protected:
    uint16_t get_help_h();
};
