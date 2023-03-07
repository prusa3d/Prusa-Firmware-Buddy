/**
 * @file screen_menu_connect.cpp
 */

#include "screen_menu_connect.hpp"
#include "printers.h"
#include <window_msgbox.hpp>
#include <connect/connect.hpp>
#include <connect/marlin_printer.hpp>

using connect_client::OnlineStatus;

MI_CONNECT_ENABLED::MI_CONNECT_ENABLED()
    : WI_ICON_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_CONNECT_ENABLED), string_view_utf8::MakeCPUFLASH((const uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_CONNECT_ENABLED::OnChange(size_t old_index) {
    eeprom_set_var(EEVAR_CONNECT_ENABLED, variant8_bool(index));
    // Connect will catch up with new config in its next iteration
}

MI_CONNECT_STATUS::MI_CONNECT_STATUS()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_CONNECT_LOAD_SETTINGS::MI_CONNECT_LOAD_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::no) {}

void MI_CONNECT_LOAD_SETTINGS::click(IWindowMenu &window_menu) {
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

#define S(STATUS, TEXT)                                    \
    case OnlineStatus::STATUS:                             \
        Item<MI_CONNECT_STATUS>().ChangeInformation(TEXT); \
        break;

void ScreenMenuConnect::updateStatus() {
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
        // These are unlikely to be shown, we have another screen when
        // registering. But we shall cover the complete set anyway.
        S(RegistrationRequesting, _("Registering"));
        S(RegistrationCode, _("Reg. code"));
        S(RegistrationDone, _("Reg. done"));
        S(RegistrationError, _("Reg. error"));
    default:
        S(Unknown, _("Unknown"));
    }
}

ScreenMenuConnect::ScreenMenuConnect()
    : ScreenMenuConnect__(_(label)) {
}

void ScreenMenuConnect::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK || event == GUI_event_t::LOOP) {
        updateStatus();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}
