/**
 * @file screen_menu_connect.cpp
 */

#include "screen_menu_connect.hpp"
#include "dialogs/DialogConnectReg.hpp"
#include "printers.h"
#include <window_msgbox.hpp>
#include <connect/connect.hpp>
#include <connect/marlin_printer.hpp>
#include <config_store/store_instance.hpp>

using connect_client::ConnectionStatus;
using connect_client::OnlineError;

MI_CONNECT_ENABLED::MI_CONNECT_ENABLED()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().connect_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_CONNECT_ENABLED::OnChange([[maybe_unused]] size_t old_index) {
    config_store().connect_enabled.set(static_cast<bool>(index));
    // Connect will catch up with new config in its next iteration
}

MI_CONNECT_STATUS::MI_CONNECT_STATUS()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_CONNECT_ERROR::MI_CONNECT_ERROR()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_CONNECT_LOAD_SETTINGS::MI_CONNECT_LOAD_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::no) {}

void MI_CONNECT_LOAD_SETTINGS::click([[maybe_unused]] IWindowMenu &window_menu) {
    if (connect_client::MarlinPrinter::load_cfg_from_ini()) {
        if (config_store().connect_enabled.get()) {
            MsgBoxInfo(_("Loaded successfully. Connect will activate shortly."), Responses_Ok);
        } else {
            MsgBoxInfo(_("Loaded successfully. Enable Connect to activate."), Responses_Ok);
        }
    } else {
        MsgBoxError(_("Failed to load config. Make sure the ini file downloaded from Connect is on the USB drive and try again."), Responses_Ok);
    }
}

MI_CONNECT_REGISTER::MI_CONNECT_REGISTER()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::no) {
}

void MI_CONNECT_REGISTER::click([[maybe_unused]] IWindowMenu &window_menu) {
    DialogConnectRegister::Show();
}

#define S(STATUS, TEXT)                                    \
    case ConnectionStatus::STATUS:                         \
        Item<MI_CONNECT_STATUS>().ChangeInformation(TEXT); \
        break;

#define E(ERR, TEXT)                                      \
    case OnlineError::ERR:                                \
        Item<MI_CONNECT_ERROR>().ChangeInformation(TEXT); \
        break;

void ScreenMenuConnect::updateStatus() {
    const auto status = connect_client::last_status();
    switch (get<0>(status)) {
        S(Off, _("Off"));
        S(NoConfig, _("No Config"));
        S(Error, _("Error"));
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
    switch (get<1>(status)) {
        E(NoError, _("---"));
        E(Dns, _("DNS error"));
        E(Connection, _("Refused"));
        E(Tls, _("TLS error"));
        E(Auth, _("Unauthorized"));
        E(Server, _("Srv error"));
        E(Internal, _("Bug"));
        E(Network, _("Net fail"));
        E(Confused, _("Protocol err"));
    }
    // Make sure that if the connect is enabled as part of the wizard, this
    // gets reflected on the toggle.
    //
    // It's kind of stupid to do this repeatedly even though this changes only
    // as a result of the MI_CONNECT_REGISTER::click. But that one doesn't have
    // access to our items (AFAIK).
    //
    // It should be cheap anyway - both the eeprom access is cached in RAM and
    // the SetIndex checks it is different before doing anything.
    Item<MI_CONNECT_ENABLED>().SetIndex(config_store().connect_enabled.get());
}

#undef S
#undef E

ScreenMenuConnect::ScreenMenuConnect()
    : ScreenMenuConnect__(_(label)) {
    // Make the status info available right from the start, do not wait for the first loop
    updateStatus();
}

void ScreenMenuConnect::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK || event == GUI_event_t::LOOP) {
        updateStatus();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}
