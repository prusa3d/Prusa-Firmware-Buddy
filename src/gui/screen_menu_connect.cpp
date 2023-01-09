#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "printers.h"
#include <window_msgbox.hpp>
#include <connect/connect.hpp>
#include <connect/marlin_printer.hpp>

using connect_client::OnlineStatus;

class MI_CONNECT_ENABLED : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *label = N_("Enabled");

public:
    MI_CONNECT_ENABLED()
        : WI_ICON_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_CONNECT_ENABLED), string_view_utf8::MakeCPUFLASH((const uint8_t *)label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override {
        eeprom_set_var(EEVAR_CONNECT_ENABLED, variant8_bool(index));
        // Connect will catch up with new config in its next iteration
    }
};

class MI_CONNECT_STATUS : public WI_INFO_t {
    constexpr static const char *const label = N_("Status");

public:
    MI_CONNECT_STATUS()
        : WI_INFO_t(_(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {
    }
};

class MI_CONNECT_LOAD_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Load settings");

public:
    MI_CONNECT_LOAD_SETTINGS()
        : WI_LABEL_t(_(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no, expands_t::no) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        if (connect_client::MarlinPrinter::load_cfg_from_ini()) {
            if (eeprom_get_bool(EEVAR_CONNECT_ENABLED)) {
                MsgBoxInfo(_("Loaded successfully. Connect will activate shortly."), Responses_Ok);
            } else {
                MsgBoxInfo(_("Loaded successfully. Enable Connect to activate."), Responses_Ok);
            }
        } else {
            MsgBoxError(_("Failed to load config. Make sure the ini file downloaded from Connect is on the USB drive and try again."), Responses_Ok);
        }
    }
};

using Screen = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_CONNECT_ENABLED, MI_CONNECT_STATUS, MI_CONNECT_LOAD_SETTINGS>;

#define S(STATUS, TEXT)                                    \
    case OnlineStatus::STATUS:                             \
        Item<MI_CONNECT_STATUS>().ChangeInformation(TEXT); \
        break;

class ScreenMenuConnect : public Screen {
private:
    void updateStatus() {
        switch (connect_client::last_status()) {
            S(Off, _("Off"));
            S(NoConfig, _("No Config"));
            S(NoDNS, _("DNS error"));
            S(NoConnection, _("Refused"));
            S(Tls, _("TLS error"));
            S(Auth, _("Unauthorized"));
            S(ServerError, _("Srv error"));
            S(InternalError, _("Bug"));
            S(NetworkError, _("Net fail"));
            S(Confused, _("Protocol err"));
            S(Ok, _("Online"));
            S(Connecting, _("Connecting"));
        default:
            S(Unknown, _("Unknown"));
        }
    }

public:
    constexpr static const char *label = N_("PRUSA CONNECT");
    ScreenMenuConnect()
        : Screen(_(label)) {
    }
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override {
        if (event == GUI_event_t::CHILD_CLICK || event == GUI_event_t::LOOP) {
            updateStatus();
        } else {
            SuperWindowEvent(sender, event, param);
        }
    }
};

ScreenFactory::UniquePtr GetScreenMenuConnect() {
    return ScreenFactory::Screen<ScreenMenuConnect>();
}
