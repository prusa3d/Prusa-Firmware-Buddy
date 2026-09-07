#include "screen_network_setup.hpp"
#include "netdev.h"

#include <espif.h>
#include <window_menu_virtual.hpp>
#include <window_menu_adv.hpp>
#include <i_window_menu_item.hpp>
#include <img_resources.hpp>
#include <dialog_text_input.hpp>
#include <raii/scope_guard.hpp>
#include <window_menu.hpp>
#include <WinMenuContainer.hpp>
#include <fsm_menu_item.hpp>
#include <fsm_network_setup.hpp>
#include <logging/log.hpp>
#include "timing.h"
#include <DialogConnectReg.hpp>
#include <gui/standard_frame/frame_prompt.hpp>
#include <gui/standard_frame/frame_qr_prompt.hpp>

#if HAS_NFC()
    #include <nfc.hpp>
#endif

LOG_COMPONENT_REF(GUI);

namespace network_wizard {

using Phase = PhaseNetworkSetup;

const char *text_password = N_("Password for %s");
const char *text_return = N_("Return");

bool display_password_input(std::span<const char> ssid, std::span<char> password) {
    StringViewUtf8Parameters<config_store_ns::wifi_max_ssid_len + 1> params;
    auto prompt = _(text_password).formatted(params, ssid.data());

    // if SSID is too long, use default
    if (prompt.computeNumUtf8Chars() > DialogTextInput::get_max_prompt_length()) {
        _(text_password).formatted(params, "Wi-fi");
    }

    return DialogTextInput::exec(prompt, password);
};

class MI_ACTION_RETURN : public FSMMenuItem {

public:
    MI_ACTION_RETURN()
        : FSMMenuItem(Phase::action_select, Response::Back, _("Return"), &img::folder_up_16x16) {}
};

class MI_ACTION_SKIP : public FSMMenuItem {

public:
    MI_ACTION_SKIP()
        : FSMMenuItem(Phase::action_select, Response::Back, _("Skip Network Setup"), &img::cancel_16x16) {}
};

class MI_ACTION_HELP : public FSMMenuItem {

public:
    MI_ACTION_HELP()
        : FSMMenuItem(Phase::action_select, Response::Help, _("Help"), &img::question_16x16) {}
};

class MI_ACTION_LOAD_INI : public FSMMenuItem {

public:
    MI_ACTION_LOAD_INI()
        : FSMMenuItem(Phase::action_select, FSMResponseVariant::make(NetworkSetupResponse::load_from_ini), _("Load Config From File")) {}
};

#if HAS_NFC()
class MI_ACTION_LOAD_NFC : public FSMMenuItem {

public:
    MI_ACTION_LOAD_NFC()
        : FSMMenuItem(Phase::action_select, FSMResponseVariant::make(NetworkSetupResponse::scan_nfc), _("Setup Via NFC")) {
        set_is_hidden(!nfc::has_nfc_probably());
    }
};
#endif

class MI_ACTION_SCAN : public FSMMenuItem {

public:
    MI_ACTION_SCAN()
        : FSMMenuItem(Phase::action_select, FSMResponseVariant::make(NetworkSetupResponse::scan_wifi), _("Scan Networks")) {}
};

class MI_ACTION_MANUAL : public IWindowMenuItem {

public:
    MI_ACTION_MANUAL()
        : IWindowMenuItem(_("Enter Credentials Manually")) {}

protected:
    virtual void click(IWindowMenu &) {
        std::array<char, config_store_ns::wifi_max_ssid_len + 1> ssid = config_store().wifi_ap_ssid.get();
        if (!DialogTextInput::exec(_("SSID"), ssid)) {
            return;
        }

        std::array<char, config_store_ns::wifi_max_passwd_len + 1> password = { 0 };

        if (!display_password_input(ssid, password)) {
            return;
        }

        config_store().wifi_ap_ssid.set(ssid);
        config_store().wifi_ap_password.set(password);
        marlin_client::FSM_response_variant(Phase::action_select, FSMResponseVariant::make(NetworkSetupResponse::connect));
    }
};

class FrameActionSelect {

public:
    FrameActionSelect(window_t *parent)
        : menu(parent, parent->GetRect(), &container) //
    {
        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(menu);
    }

    void update(fsm::PhaseData data) {
        const WizardMode mode = static_cast<WizardMode>(data[0]);
        container.Item<MI_ACTION_RETURN>().set_is_hidden(mode != WizardMode::from_network_menu);
        container.Item<MI_ACTION_SKIP>().set_is_hidden(mode != WizardMode::initial_setup);
    }

private:
    WinMenuContainer<
        MI_ACTION_RETURN,
        MI_ACTION_SCAN,
        MI_ACTION_MANUAL,
#if HAS_NFC()
        MI_ACTION_LOAD_NFC,
#endif
        MI_ACTION_LOAD_INI,
        MI_ACTION_HELP,
        MI_ACTION_SKIP //
        >
        container;

    window_menu_t menu;
};
static_assert(common_frames::is_update_callable<FrameActionSelect>);

class MI_SCAN_RETURN : public FSMMenuItem {

public:
    MI_SCAN_RETURN()
        : FSMMenuItem(Phase::wifi_scan, Response::Back, _("Return"), &img::folder_up_16x16) {}
};

class MI_WIFI : public IWindowMenuItem {

public:
    MI_WIFI(int wifi_index, const char *ssid, bool needs_password)
        : IWindowMenuItem({}, needs_password ? &img::wifi_16x16 : &img::wifi_insecure_16x16)
        , wifi_index_(wifi_index)
        , needs_password_(needs_password) {
        strlcpy(ssid_.data(), ssid, ssid_.size());
        SetLabel(string_view_utf8::MakeRAM(ssid_.data()));
    }

    inline int wifi_index() const {
        return wifi_index_;
    }

protected:
    virtual void click(IWindowMenu &) {
        std::array<char, config_store_ns::wifi_max_passwd_len + 1> password = { 0 };

        if (needs_password_ && !display_password_input(ssid_, password)) {
            return;
        }

        config_store().wifi_ap_ssid.set(ssid_);
        config_store().wifi_ap_password.set(password);
        marlin_client::FSM_response(Phase::wifi_scan, Response::Continue);
    }

private:
    std::array<char, config_store_ns::wifi_max_ssid_len + 1> ssid_;
    const int wifi_index_;
    const bool needs_password_;
};

class WindowMenuWifiScan : public WindowMenuVirtual {

public:
    WindowMenuWifiScan(window_t *parent, Rect16 rect)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::no) {
        if (espif::scan::start() != ERR_OK) {
            log_error(GUI, "Scan start failed");
        }
        setup_items();
    }

    ~WindowMenuWifiScan() {
        std::ignore = espif::scan::stop();
    }

public:
    int item_count() const final {
        // +1 because of the back button
        return 1 + std::max<int>(ap_count_, 1);
    }

protected:
    void setup_item(ItemVariant &variant, int index) final {
        if (index == 0) {
            variant.emplace<MI_SCAN_RETURN>();
            return;
        }

        // No networks shown -> we're showing one empty item indicating that we're scanning instead
        if (ap_count_ == 0) {
            variant.emplace<WindowMenuItem>(TERN(HAS_LARGE_DISPLAY(), _("Scanning for networks..."), _("Scanning...")), nullptr, is_enabled_t::no);
            return;
        }

        // -1 because of the return button
        const int wifi_index = index - 1;

        // The item is already set to the right wifi item -> do nothing
        // We do this to prevent calling expensive espif_scan_get_ap_ssid unnecesarily
        // This optimization makes sense when all items are re-setup when new wi-fi appears
        if (variant.holds_alternative<MI_WIFI>() && variant.get<MI_WIFI>().wifi_index() == wifi_index) {
            return;
        }

        std::array<char, config_store_ns::wifi_max_ssid_len + 1> ssid;
        bool needs_password;
        const auto ap_info_result = espif::scan::get_ap_info(wifi_index, std::bit_cast<std::span<uint8_t>>(std::span<const char>(ssid)), needs_password);

        auto &item = variant.emplace<MI_WIFI>(wifi_index, ssid.data(), needs_password);

        if (ap_info_result != ERR_OK) {
            item.SetLabel(string_view_utf8::MakeCPUFLASH("##ERROR##"));
            item.set_enabled(false);
        }
    }

    void windowEvent(window_t *sender, GUI_event_t event, void *param) {
        switch (event) {

        case GUI_event_t::LOOP: {
            const auto current_ms = ticks_ms();
            if (ticks_diff(current_ms, last_wifi_count_check_ms_) > 1000) {
                last_wifi_count_check_ms_ = current_ms;
                const auto new_ap_count = espif::scan::get_ap_count();

                if (new_ap_count != ap_count_) {
                    ap_count_ = new_ap_count;

                    // Notify that the items changed and we potentially need to create new items
                    setup_items();
                }
            }

            if (IWindowMenuItem * item; ap_count_ == 0 && (item = item_at(1))) {
                item->SetIconId(img::spinner_16x16_animated());
            }
            break;
        }

        default:
            break;
        }

        WindowMenuVirtual::windowEvent(sender, event, param);
    }

private:
    uint32_t last_wifi_count_check_ms_ = 0;
    uint8_t ap_count_ = 0;
};

class FrameWifiScan {

public:
    FrameWifiScan(window_frame_t *parent)
        : menu(parent, parent->GetRect()) {

        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(menu);
    }

private:
    WindowExtendedMenu<WindowMenuWifiScan> menu;
};

class FrameAskSwitchToWifi : public FramePrompt {

public:
    FrameAskSwitchToWifi(window_frame_t *parent)
        : FramePrompt(parent, Phase::ask_switch_to_wifi, _("Switch to Wi-Fi"), _("You're already successfully connected through the ethernet cable.\nSwitch to Wi-Fi and continue?")) {
    }
};

class FrameTextWithSSID : public FramePrompt {
public:
    FrameTextWithSSID(window_frame_t *parent, Phase phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info)
        : FramePrompt(parent, phase, txt_title, txt_info) {
        info.SetText(txt_info.formatted(params, config_store().wifi_ap_ssid.get_c_str()));
    }

protected:
    StringViewUtf8Parameters<config_store_ns::wifi_max_ssid_len + 1> params;
};

class FrameConnectingFinishable : public FrameTextWithSSID {

public:
    FrameConnectingFinishable(window_frame_t *parent)
        : FrameTextWithSSID(parent, Phase::connecting_finishable, _("Connecting"), _("SSID: %s\n\nYou can press 'Finish' to continue connecting on the background.")) {}
};

class FrameConnectingNonfinishable : public FrameTextWithSSID {

public:
    FrameConnectingNonfinishable(window_frame_t *parent)
        : FrameTextWithSSID(parent, Phase::connecting_nonfinishable, _("Connecting"), _("SSID: %s")) {}
};

class FrameESPError : public FramePrompt {

public:
    FrameESPError(window_frame_t *parent)
        : FramePrompt(parent, Phase::no_interface_error, _("No network interface"), _("The Wi-Fi module is not working properly or is missing.\n\nInsert the module, try restarting the printer, or use the ethernet cable.")) {
    }
};

class FrameError : public FramePrompt {

public:
    FrameError(window_frame_t *parent)
        : FramePrompt(parent, Phase::connection_error, _("Error"), _("There was an error connecting to the Wi-Fi.")) {
    }
};

class FrameConnected final : public FramePrompt {
public:
    FrameConnected(window_frame_t *parent)
        : FramePrompt {
            parent,
            Phase::connected,
            _("Successfully connected"),
            _("SSID: %s\n\nYou can now fully use all network features of the printer."),
        } {
        const char *ssid = (config_store().active_netdev.get() == NETDEV_ESP_ID)
            ? config_store().wifi_ap_ssid.get_c_str()
            : "Ethernet";
        info.SetText(info.GetText().formatted(params, ssid));
    }

private:
    StringViewUtf8Parameters<config_store_ns::wifi_max_ssid_len + 1> params;
};

class FrameWaitForINI : public FramePrompt {

public:
    FrameWaitForINI(window_frame_t *parent)
        : FramePrompt(parent, Phase::wait_for_ini_file, _("Credentials from INI"), _("Please insert a flash drive with a network configuration file.\n\nThe configuration file can be generated in PrusaSlicer.")) {
    }
};

class FrameAskDeleteINIFile : public FramePrompt {

public:
    FrameAskDeleteINIFile(window_frame_t *parent)
        : FramePrompt(parent, Phase::ask_delete_ini_file, _("Delete INI file"), _("Delete credentials INI file? (Recommended)")) {
    }
};

using FrameHelpQR = WithConstructorArgs<FrameQRPrompt, Phase::help_qr, N_("To setup or troubleshoot your Wi-Fi, please visit:"), "wifi"_tstr>;

#if HAS_NFC()

using FrameAskUsePrusaApp = WithConstructorArgs<FrameQRPrompt, Phase::ask_use_prusa_app, N_("Do you want to connect to the Wi-Fi with the Prusa app on your phone using NFC?"), "app"_tstr>;

class FrameWaitForNFC : public FramePrompt {
    nfc::SharedEnabler nfc_enable;

public:
    FrameWaitForNFC(window_frame_t *parent)
        : FramePrompt(parent, Phase::wait_for_nfc,
            _("Credentials via NFC"),
            _("1. Open Prusa app on your mobile device.\n\n2. Go to in-app Menu and select \"Set up Printer Wi-Fi.\"\n\n3. Follow on-screen instructions."),
            Align_t::LeftTop(), Align_t::LeftBottom()) {
    }
};

class FrameConfirmNFC : public FramePrompt {

public:
    FrameConfirmNFC(window_frame_t *parent)
        : FramePrompt(parent, Phase::nfc_confirm, _("Credentials via NFC"), {}) {
        static constexpr const char *wifi_credentials_loaded_txt = N_("Wi-Fi credentials loaded via NFC.\nApply credentials?\n\nSSID: %s");
        string_view_utf8 str;
        marlin_vars().generic_param_string.execute_with([&](const auto &param) {
            str = _(wifi_credentials_loaded_txt).formatted(string_view_parameters, param);
        });

        info.SetText(str);
    }

protected:
    StringViewUtf8Parameters<33> string_view_parameters;
};
#endif

class FrameAskSetupPrusaConnect : public FramePrompt {

public:
    FrameAskSetupPrusaConnect(window_frame_t *parent)
        : FramePrompt(parent, Phase::ask_setup_prusa_connect, _("Set up Prusa Connect?"), _("Do you want to add your printer to Prusa Connect?")) {
    }
};

class FramePrusaConnectSetup {

public:
    FramePrusaConnectSetup(window_t *) {
        // Prusa Connect setup is a dialog for some reason, so just blockingly execute it
        DialogConnectRegister::Show();

        // And let the FSM now that we finished afterwards
        marlin_client::FSM_response(PhaseNetworkSetup::prusa_conect_setup, Response::Done);
    }
};

using Frames = FrameDefinitionList<ScreenNetworkSetup::FrameStorage,
#if HAS_NFC()
    FrameDefinition<Phase::ask_use_prusa_app, FrameAskUsePrusaApp>,
    FrameDefinition<Phase::wait_for_nfc, FrameWaitForNFC>,
    FrameDefinition<Phase::nfc_confirm, FrameConfirmNFC>,
#endif
#if BUDDY_ENABLE_CONNECT()
    FrameDefinition<Phase::ask_setup_prusa_connect, FrameAskSetupPrusaConnect>,
    FrameDefinition<Phase::prusa_conect_setup, FramePrusaConnectSetup>,
#endif
    FrameDefinition<Phase::ask_switch_to_wifi, FrameAskSwitchToWifi>,
    FrameDefinition<Phase::action_select, FrameActionSelect>,
    FrameDefinition<Phase::wifi_scan, FrameWifiScan>,
    FrameDefinition<Phase::ask_delete_ini_file, FrameAskDeleteINIFile>,
    FrameDefinition<Phase::wait_for_ini_file, FrameWaitForINI>,
    FrameDefinition<Phase::connecting_finishable, FrameConnectingFinishable>,
    FrameDefinition<Phase::connecting_nonfinishable, FrameConnectingNonfinishable>,
    FrameDefinition<Phase::no_interface_error, FrameESPError>,
    FrameDefinition<Phase::connection_error, FrameError>,
    FrameDefinition<Phase::help_qr, FrameHelpQR>,
    FrameDefinition<Phase::connected, FrameConnected> //
    >;

} // namespace network_wizard

using namespace network_wizard;

ScreenNetworkSetup::ScreenNetworkSetup()
    : ScreenFSM(N_("NETWORK SETUP"), GuiDefaults::RectScreenNoHeader) {
    header.SetIcon(&img::wizard_16x16);
    create_frame();
}

ScreenNetworkSetup::~ScreenNetworkSetup() {
    destroy_frame();
}

void ScreenNetworkSetup::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}
void ScreenNetworkSetup::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}
void ScreenNetworkSetup::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}

void ScreenNetworkSetup::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        marlin_client::FSM_response(get_phase(), Response::Back);
        return;

    default:
        break;
    }

    ScreenFSM::screenEvent(sender, event, param);
}
