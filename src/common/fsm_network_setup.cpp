#include "fsm_network_setup.hpp"

#include <settings_ini.hpp>
#include <netdev.h>
#include <wui.h>
#include <espif.h>
#include <fsm_handler.hpp>
#include <common/marlin_server.hpp>
#include <sys/stat.h>
#include <option/has_nfc.h>
#include <connect/connect.hpp>

#if HAS_NFC()
    #include <nfc.hpp>
#endif

namespace network_wizard {

class FSMNetworkSetup final {

public:
    using Phase = PhaseNetworkSetup;
    using PhaseOpt = std::optional<Phase>;

    using Config = FSMHandlerPhasesConfig<Phase, FSMNetworkSetup>;
    using Meta = FSMHandlerMetadata<Phase, FSMNetworkSetup>;
    using PhaseConfig = FSMHandlerPhaseConfig<Phase, FSMNetworkSetup>;

public:
    FSMNetworkSetup()
        : fsm_(*this) {}

    void setup_initial_setup() {
        mode_ = WizardMode::initial_setup;

        // We cannot allow continuing the connecting on background on the initial setup,
        // because we want to offer Prusa Connect if the user connects successfully.
        connecting_phase_ = Phase::connecting_nonfinishable;
    }

    void setup_ini() {
        mode_ = WizardMode::ini_load_only;
        first_phase_ = Phase::wait_for_ini_file;
        cancel_target_phase_ = Phase::finish;
    }

#if HAS_NFC()
    void setup_nfc_only(const WifiCredentials &credentials) {
        mode_ = WizardMode::nfc_only;
        nfc_credentials_ = credentials;
        first_phase_ = Phase::nfc_confirm;
        cancel_target_phase_ = Phase::finish;
    }
#endif

    inline bool loop() {
        return fsm_.loop();
    }

    void run() {
        while (loop()) {
            idle(true);
        }
    }

private:
    PhaseOpt check_nfc() {
#if HAS_NFC()
        const auto current_ms = ticks_ms();
        if (ticks_diff(current_ms, last_nfc_check_ms_) < nfc::OPTIMAL_CHECK_DIFF_MS) {
            return std::nullopt;
        }
        last_nfc_check_ms_ = current_ms;

        if (!nfc::has_activity()) {
            return std::nullopt;
        }

        const auto credentials = nfc::consume_data();

        if (!credentials) {
            return std::nullopt;
        }

        nfc_credentials_ = *credentials;
        return Phase::nfc_confirm;
#else
        return std::nullopt;
#endif
    }

    PhaseOpt phase_init(const Meta::LoopCallbackArgs &) {
        const auto active_interface = config_store().active_netdev.get();

        // Initial setup only cares about being connected to the internet. If we already are, just show it.
        if (mode_ == WizardMode::initial_setup && netdev_get_status(active_interface) == NETDEV_NETIF_UP) {
            return Phase::connected;
        }

        const auto esp_state = esp_fw_state();
        if (esp_state != EspFwState::Ok && esp_state != EspFwState::Scanning) {
            return Phase::no_interface_error;
        }

        // If we're successfully connected through ethernet, first ask if want to switch to wi-fi
        if (active_interface != NETDEV_ESP_ID) {
            return Phase::ask_switch_to_wifi;
        }

#if HAS_NFC()
        // Offer using Prusa app only to printers that actually have NFC. Without it, the app is pointless.
        if (mode_ == WizardMode::initial_setup && nfc::has_nfc_probably()) {
            return Phase::ask_use_prusa_app;
        }
#endif

        return first_phase_;
    }

    PhaseOpt phase_ask_switch_to_wifi(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Yes:
            // It takes a bit to apply the change
            netdev_set_active_id(NETDEV_ESP_ID);
            return Phase::init;

        case Response::No:
        case Response::Back: // From touch swipe
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    void phase_action_select_init(const Meta::InitCallbackArgs &) {
        // The UI is a bit different based on the wizard mode
        fsm_.change_data(fsm::PhaseData { static_cast<uint8_t>(mode_) });
    }

    PhaseOpt phase_action_select(const Meta::LoopCallbackArgs &args) {
        if (auto phase = check_nfc()) {
            return phase;
        }

        switch (args.response.value_or(Response::_none)) {

        case Response::Back:
            return Phase::finish;

        case Response::Help:
            return Phase::help_qr;

        default:
            break;
        }

        switch (args.response.value_or(NetworkSetupResponse::_count)) {

        case NetworkSetupResponse::scan_wifi:
            return Phase::wifi_scan;

        case NetworkSetupResponse::load_from_ini:
            return Phase::wait_for_ini_file;

        case NetworkSetupResponse::connect:
            // Continue -> user set up the new credentials into the config_store
            return connecting_phase_;

#if HAS_NFC()
        case NetworkSetupResponse::scan_nfc:
            return Phase::wait_for_nfc;
#endif
        default:
            break;
        }

        return std::nullopt;
    }

    PhaseOpt phase_wifi_scan(const Meta::LoopCallbackArgs &args) {
        if (auto phase = check_nfc()) {
            return phase;
        }

        // Wi-fi scanning is done fully in the UI.
        // The FSM waits for the UI to write into config_store wifi credentials
        // and send NetworkSetupResponse::connect response.

        switch (args.response.value_or(Response::_none)) {

        case Response::Back:
            return Phase::action_select;

        case Response::Continue:
            // Continue -> user set up the new credentials into the config_store
            return connecting_phase_;

        default:
            return std::nullopt;
        }
    }

    void phase_general_init(const Meta::InitCallbackArgs &) {
        phase_start_ms_ = ticks_ms();
        phase_action_done_ = false;
    }

    PhaseOpt phase_wait_for_ini_file(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Back: // From touch swipe
        case Response::Cancel:
            return Phase::action_select;

        default:
            break;
        }

        const auto phase_time_ms = ticks_diff(ticks_ms(), phase_start_ms_);

        // File exists & was successfully loaded -> go to the connecting phase
        // We do this after some delay, because it freezes the printer.
        // If we did this in the phase init, the screen would not be redrawn for some time.
        if (struct stat st; !phase_action_done_ && phase_time_ms > 500 && stat(settings_ini::file_name, &st) == 0 && S_ISREG(st.st_mode) && netdev_load_ini_to_eeprom()) {
            phase_action_done_ = true;
            return Phase::ask_delete_ini_file;
        }

        return std::nullopt;
    }

    PhaseOpt phase_ask_delete_ini_file(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Back: // From touch swipe
            return Phase::action_select;

        case Response::Yes:
            remove(settings_ini::file_name);
            return connecting_phase_;

        case Response::No:
            return connecting_phase_;

        default:
            break;
        }

        return std::nullopt;
    }

#if HAS_NFC()
    PhaseOpt phase_ask_use_prusa_app(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Yes:
            return Phase::wait_for_nfc;

        case Response::No:
            return Phase::action_select;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_wait_for_nfc(const Meta::LoopCallbackArgs &args) {
        if (auto phase = check_nfc()) {
            return phase;
        }

        switch (args.response.value_or(Response::_none)) {

        case Response::Back: // From touch swipe
        case Response::Cancel:
            return Phase::action_select;

        default:
            break;
        }

        return std::nullopt;
    }

    void phase_nfc_confirm_init(const Meta::InitCallbackArgs &) {
        marlin_vars().generic_param_string.set(nfc_credentials_.ssid.data(), nfc_credentials_.ssid.size());
    }

    PhaseOpt phase_nfc_confirm(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Ok:
            config_store().wifi_ap_ssid.set(nfc_credentials_.ssid.data());
            config_store().wifi_ap_password.set(nfc_credentials_.password.data());
            return connecting_phase_;

        case Response::Back: // From touch swipe
        case Response::Cancel:
            return cancel_target_phase_;

        default:
            return std::nullopt;
        }
    }
#endif

    void phase_connecting_init(const Meta::InitCallbackArgs &args) {
        phase_general_init(args);
        netdev_set_active_id(NETDEV_ESP_ID);
    }

    /// Commit the network configuration. This blocks for some significant time.
    void commit_network_configuration() {
        espif_reset_connection();
        notify_reconfigure();
    }

    void phase_connecting_exit(const Meta::ExitCallbackArgs &) {
        // If the user closed the screen before the netif update was issued, issue it now.
        if (!phase_action_done_) {
            commit_network_configuration();
        }
    }

    PhaseOpt phase_connecting(const Meta::LoopCallbackArgs &args) {
        if (phase_action_done_ && netdev_get_status(NETDEV_ESP_ID) == NETDEV_NETIF_UP) {
            return Phase::connected;
        }

        const auto phase_time_ms = ticks_diff(ticks_ms(), phase_start_ms_);

        // Notify the networking stack to reload the configuration
        // We do this after some delay, because it freezes the printer.
        // If we did this in the phase init, the screen would not be redrawn for some time.
        if (phase_time_ms > 700 && !phase_action_done_) {
            commit_network_configuration();
            phase_action_done_ = true;
        }

        // Connecting takes too long -> go to the error screen
        if (phase_time_ms > 120 * 1000) {
            return Phase::connection_error;
        }

        switch (args.response.value_or(Response::_none)) {

        case Response::Back:
        case Response::Cancel:
            return cancel_target_phase_;

        case Response::Finish:
        case Response::Abort:
            return Phase::finish;

        default:
            break;
        }

        return std::nullopt;
    }

    PhaseOpt phase_connected(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Ok:
            if (mode_ == WizardMode::initial_setup && std::get<0>(connect_client::last_status()) != connect_client::ConnectionStatus::Ok) {
                return Phase::ask_setup_prusa_connect;

            } else {
                return Phase::finish;
            }

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_ask_setup_prusa_connect(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Yes:
            return Phase::prusa_conect_setup;

        case Response::No:
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_prusa_connect_setup(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Done:
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_esp_error(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Retry:
            return Phase::init;

        case Response::Help:
            return Phase::help_qr;

        case Response::Ok:
        case Response::Back: // From touch swipe
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_connecting_error(const Meta::LoopCallbackArgs &args) {
        // Wi-fi decided to connect after all -> jump to connected
        if (netdev_get_status(NETDEV_ESP_ID) == NETDEV_NETIF_UP) {
            return Phase::connected;
        }

        switch (args.response.value_or(Response::_none)) {

        case Response::Back:
            return cancel_target_phase_;

        case Response::Help:
            return Phase::help_qr;

        case Response::Abort:
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_help_qr(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or(Response::_none)) {

        case Response::Back:
            return Phase::init;

        default:
            return std::nullopt;
        }
    }

private:
    using C = FSMNetworkSetup;
    static constexpr Config config {
        { Phase::init, { &C::phase_init } },
            { Phase::ask_switch_to_wifi, { &C::phase_ask_switch_to_wifi } },
            { Phase::action_select, { .loop_callback = &C::phase_action_select, .init_callback = &C::phase_action_select_init } },
            { Phase::wifi_scan, { .loop_callback = &C::phase_wifi_scan } },
            { Phase::wait_for_ini_file, { .loop_callback = &C::phase_wait_for_ini_file, .init_callback = &C::phase_general_init } },
            { Phase::ask_delete_ini_file, { .loop_callback = &C::phase_ask_delete_ini_file } },
#if HAS_NFC()
            { Phase::ask_use_prusa_app, { &C::phase_ask_use_prusa_app } },
            { Phase::wait_for_nfc, { &C::phase_wait_for_nfc } },
            { Phase::nfc_confirm, { .loop_callback = &C::phase_nfc_confirm, .init_callback = &C::phase_nfc_confirm_init } },
#endif
            { Phase::connecting_finishable, { .loop_callback = &C::phase_connecting, .init_callback = &C::phase_connecting_init, .exit_callback = &C::phase_connecting_exit } },
            { Phase::connecting_nonfinishable, { .loop_callback = &C::phase_connecting, .init_callback = &C::phase_connecting_init, .exit_callback = &C::phase_connecting_exit } },
            { Phase::connected, { &C::phase_connected } },

            { Phase::ask_setup_prusa_connect, { &C::phase_ask_setup_prusa_connect } },
            { Phase::prusa_conect_setup, { &C::phase_prusa_connect_setup } },

            { Phase::no_interface_error, { &C::phase_esp_error } },
            { Phase::connection_error, { &C::phase_connecting_error } },
            { Phase::help_qr, { &C::phase_help_qr } },
            { Phase::finish, {} },
    };

private:
    FSMHandler<config> fsm_;

    WizardMode mode_ = WizardMode::from_network_menu;

    // Used only in some phases
    uint32_t phase_start_ms_;

    /// First phase the wizard should go to after init
    Phase first_phase_ = Phase::action_select;

    /// Phase the FSM should go to when cancel/back is pressed
    Phase cancel_target_phase_ = Phase::action_select;

    /// Phase the FSM should go to when connecting
    Phase connecting_phase_ = Phase::connecting_finishable;

#if HAS_NFC()
    /// Time of the last nfc check
    uint32_t last_nfc_check_ms_ = 0;

    /// Credentials scanned through NFC
    WifiCredentials nfc_credentials_;
#endif

    bool phase_action_done_;
};

void network_setup_wizard() {
    FSMNetworkSetup fsm;
    fsm.run();
}

void network_ini_wizard() {
    FSMNetworkSetup fsm;
    fsm.setup_ini();
    fsm.run();
}

void network_initial_setup_wizard() {
    FSMNetworkSetup fsm;
    fsm.setup_initial_setup();
    fsm.run();
}

#if HAS_NFC()
void network_nfc_wizard(const WifiCredentials &creds) {
    FSMNetworkSetup fsm;
    fsm.setup_nfc_only(creds);
    fsm.run();
}
#endif

} // namespace network_wizard
