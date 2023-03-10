/**
 * @file screen_menu_connect.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"

class MI_CONNECT_ENABLED : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *label = N_("Enabled");

public:
    MI_CONNECT_ENABLED();

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_CONNECT_STATUS : public WI_INFO_t {
    constexpr static const char *const label = N_("Status");

public:
    MI_CONNECT_STATUS();
};

class MI_CONNECT_LOAD_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Load settings");

public:
    MI_CONNECT_LOAD_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CONNECT_REGISTER : public WI_LABEL_t {
    static constexpr const char *const label = N_("Register");

public:
    MI_CONNECT_REGISTER();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CONNECT_CODE : public WI_INFO_t {
    constexpr static const char *const label = N_("Code");

public:
    MI_CONNECT_CODE();
};

using ScreenMenuConnect__ = ScreenMenu<EFooter::Off, MI_RETURN, MI_CONNECT_ENABLED, MI_CONNECT_STATUS, MI_CONNECT_LOAD_SETTINGS, MI_CONNECT_REGISTER, MI_CONNECT_CODE>;

#define S(STATUS, TEXT)                                    \
    case OnlineStatus::STATUS:                             \
        Item<MI_CONNECT_STATUS>().ChangeInformation(TEXT); \
        break;

class ScreenMenuConnect : public ScreenMenuConnect__ {
private:
    void updateStatus();

public:
    constexpr static const char *label = N_("PRUSA CONNECT");
    ScreenMenuConnect();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
