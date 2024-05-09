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

    void update();

protected:
    virtual void OnChange(size_t old_index) override;
};
static_assert(UpdatableMenuItem<MI_CONNECT_ENABLED>);

class MI_CONNECT_STATUS : public WI_INFO_t {
    constexpr static const char *const label = N_("Status");

public:
    MI_CONNECT_STATUS();

    void update();
};
static_assert(UpdatableMenuItem<MI_CONNECT_STATUS>);

class MI_CONNECT_ERROR : public WI_INFO_t {
    constexpr static const char *const label = N_("Error");

public:
    MI_CONNECT_ERROR();

    void update();
};
static_assert(UpdatableMenuItem<MI_CONNECT_ERROR>);

class MI_CONNECT_HOST : public WiInfo<32> {
    static constexpr const char *label = N_("Hostname");

public:
    MI_CONNECT_HOST();

    void update();
};
static_assert(UpdatableMenuItem<MI_CONNECT_HOST>);

class MI_CONNECT_LOAD_SETTINGS : public IWindowMenuItem {
    static constexpr const char *const label = N_("Load Settings");

public:
    MI_CONNECT_LOAD_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CONNECT_REGISTER : public IWindowMenuItem {
    static constexpr const char *const label = N_("Add Printer to Connect");

public:
    MI_CONNECT_REGISTER();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

using ScreenMenuConnect__ = ScreenMenu<EFooter::Off, MI_RETURN, MI_CONNECT_ENABLED, MI_CONNECT_STATUS, MI_CONNECT_ERROR, MI_CONNECT_HOST, MI_CONNECT_REGISTER, MI_CONNECT_LOAD_SETTINGS>;

class ScreenMenuConnect : public ScreenMenuConnect__ {

public:
    constexpr static const char *label = N_("PRUSA CONNECT");
    ScreenMenuConnect();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
