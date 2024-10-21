/**
 * @file screen_menu_fw_update.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include <guiconfig/guiconfig.h>

#if HAS_LARGE_DISPLAY()
class MI_ALWAYS : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Always");

public:
    MI_ALWAYS();
    virtual void OnChange(size_t old_index) override;
};

class MI_ON_RESTART : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("On Restart");

public:
    MI_ON_RESTART();
    virtual void OnChange(size_t old_index) override;
};

using ScreenMenuFwUpdate__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_ALWAYS, MI_ON_RESTART>;
class ScreenMenuFwUpdate : public ScreenMenuFwUpdate__ {

public:
    ScreenMenuFwUpdate();
};

#else

class MI_UPDATE_LABEL : public IWindowMenuItem {
    static constexpr const char *const label = N_("FW Update");

public:
    MI_UPDATE_LABEL();

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {};
};

class MI_UPDATE : public MenuItemSwitch {
    size_t init_index() const;

public:
    MI_UPDATE();

protected:
    virtual void OnChange(size_t) override;
};

using MenuFwUpdateContainer = WinMenuContainer<MI_RETURN, MI_UPDATE_LABEL, MI_UPDATE>;

class ScreenMenuFwUpdate : public screen_t {
    constexpr static const char *const label = N_("FW UPDATE");
    static constexpr size_t helper_lines = 4;
    static constexpr Font helper_font = Font::special;

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
#endif
