/**
 * @file screen_prusa_link.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include <config_store/constants.hpp>
#include <guiconfig/guiconfig.h>
#include <MItem_network.hpp>
#include <str_utils.hpp>
#include <gui/qr.hpp>

// ----------------------------------------------------------------
// GUI Prusa Link Password regenerate
class MI_PL_REGENERATE_PASSWORD : public IWindowMenuItem {
    constexpr static const char *const label = N_("Generate Password");

public:
    MI_PL_REGENERATE_PASSWORD();

protected:
    virtual void click(IWindowMenu &) override;
};

// ----------------------------------------------------------------
// GUI Prusa Link start after printer startup
class MI_PL_ENABLED : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Enabled");

public:
    MI_PL_ENABLED();

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_PL_PASSWORD_LABEL : public IWindowMenuItem {
    constexpr static const char *const label = N_("Password");

public:
    MI_PL_PASSWORD_LABEL();

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {}
};

class MI_PL_PASSWORD_VALUE : public WiInfo<config_store_ns::pl_password_size> {
public:
    MI_PL_PASSWORD_VALUE();

    void update_explicit();
};

class MI_PL_USER : public IWiInfo {
    constexpr static const char *const label = N_("User");

public:
    MI_PL_USER();
};

class MI_PL_QRCODE final : public IWindowMenuItem {
    constexpr static const char *const label = N_("QR Code");

public:
    MI_PL_QRCODE();
    void click(IWindowMenu &) final;
};

using ScreenMenuPrusaLink_ = ScreenMenu<EFooter::Off, MI_RETURN, MI_PL_ENABLED, MI_PL_USER,
#if HAS_MINI_DISPLAY()
    MI_PL_PASSWORD_LABEL,
#endif
    MI_PL_PASSWORD_VALUE,
    MI_IP4_ADDR,
    MI_HOSTNAME,
    MI_PL_REGENERATE_PASSWORD,
    MI_PL_QRCODE>;

class ScreenMenuPrusaLink : public ScreenMenuPrusaLink_ {
public:
    ScreenMenuPrusaLink();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenPrusaLinkQRCode final : public screen_t {
    window_text_t text;
    window_icon_t icon_phone;
    QRDynamicStringWindow<100> qr;

public:
    ScreenPrusaLinkQRCode();
    void windowEvent(window_t *, GUI_event_t, void *) final;
};
