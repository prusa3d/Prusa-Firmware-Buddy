/**
 * @file screen_menu_connect.cpp
 */

#include "screen_menu_connect.hpp"
#include <connect/hostname.hpp>
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

void MI_CONNECT_ENABLED::Loop() {
    // Make sure that if the connect is enabled as part of the wizard, this
    // gets reflected on the toggle.
    //
    // It's kind of stupid to do this repeatedly even though this changes only
    // as a result of the MI_CONNECT_REGISTER::click. But that one doesn't have
    // access to our items (AFAIK).
    //
    // It should be cheap anyway - both the eeprom access is cached in RAM and
    // the SetIndex checks it is different before doing anything.
    SetIndex(config_store().connect_enabled.get());
}

MI_CONNECT_STATUS::MI_CONNECT_STATUS()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_CONNECT_STATUS::Loop() {
    using S = connect_client::ConnectionStatus;
    static constexpr EnumArray<S, const char *, connect_client::connection_status_cnt> strings {
        { S::Unknown, N_("Unknown") },
        { S::Off, N_("Off") },
        { S::NoConfig, N_("No Config") },
        { S::Ok, N_("Online") },
        { S::Connecting, N_("Connecting") },
        { S::Error, N_("Error") },
        { S::RegistrationRequesting, N_("Registering") },
        { S::RegistrationCode, N_("Reg. code") },
        { S::RegistrationDone, N_("Reg. done") },
        { S::RegistrationError, N_("Reg. error") },
    };

    ChangeInformation(_(strings.get_fallback(std::get<0>(connect_client::last_status()), S::Unknown)));
}

MI_CONNECT_ERROR::MI_CONNECT_ERROR()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_CONNECT_ERROR::Loop() {
    using S = connect_client::OnlineError;
    static constexpr EnumArray<S, const char *, connect_client::online_error_cnt> strings {
        { S::NoError, N_("---") },
        { S::Dns, N_("DNS error") },
        { S::Connection, N_("Refused") },
        { S::Tls, N_("TLS error") },
        { S::Auth, N_("Unauthorized") },
        { S::Server, N_("Srv error") },
        { S::Internal, N_("Bug") },
        { S::Network, N_("Net fail") },
        { S::Confused, N_("Protocol err") },
    };

    ChangeInformation(_(strings.get_fallback(std::get<1>(connect_client::last_status()), S::Confused)));
}

MI_CONNECT_HOST::MI_CONNECT_HOST()
    : WiInfo(_(label)) {
}

void MI_CONNECT_HOST::Loop() {
    std::array<char, GetInfoLen()> hostname;
    strlcpy(hostname.data(), config_store().connect_host.get_c_str(), hostname.size());
    connect_client::decompress_host(hostname.data(), hostname.size());
    ChangeInformation(hostname.data());
}

MI_CONNECT_LOAD_SETTINGS::MI_CONNECT_LOAD_SETTINGS()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::no) {}

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
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::no) {
}

void MI_CONNECT_REGISTER::click([[maybe_unused]] IWindowMenu &window_menu) {
    DialogConnectRegister::Show();
}

ScreenMenuConnect::ScreenMenuConnect()
    : ScreenMenuConnect__(_(label)) {
}
