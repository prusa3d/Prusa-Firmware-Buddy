#include "screen_network_setup.hpp"
#include "netdev.h"

#include <espif.h>
#include <window_menu_virtual.hpp>
#include <window_menu_adv.hpp>
#include <i_window_menu_item.hpp>
#include <str_utils.hpp>
#include <img_resources.hpp>
#include <dialog_text_input.hpp>
#include <scope_guard.hpp>
#include <window_menu.hpp>
#include <WinMenuContainer.hpp>
#include <fsm_menu_item.hpp>
#include <fsm_network_setup.hpp>
#include <gui/frame_qr_layout.hpp>
#include <logging/log.hpp>
#include "timing.h"

#if HAS_NFC()
    #include <nfc.hpp>
#endif

LOG_COMPONENT_REF(GUI);

namespace network_wizard {

using Phase = PhaseNetworkSetup;

const char *text_password = N_("Password");
const char *text_return = N_("Return");

class MI_ACTION_RETURN : public FSMMenuItem {

public:
    MI_ACTION_RETURN()
        : FSMMenuItem(Phase::action_select, Response::Back, _("Return"), &img::folder_up_16x16) {}
};

class MI_ACTION_SKIP : public FSMMenuItem {

public:
    MI_ACTION_SKIP()
        : FSMMenuItem(Phase::action_select, Response::Back, _("Skip network setup"), &img::cancel_16x16) {}
};

class MI_ACTION_HELP : public FSMMenuItem {

public:
    MI_ACTION_HELP()
        : FSMMenuItem(Phase::action_select, Response::Help, _("Help"), &img::question_16x16) {}
};

class MI_ACTION_LOAD_INI : public FSMMenuItem {

public:
    MI_ACTION_LOAD_INI()
        : FSMMenuItem(Phase::action_select, FSMResponseVariant::make(NetworkSetupResponse::load_from_ini), _("Load config from file")) {}
};

#if HAS_NFC()
class MI_ACTION_LOAD_NFC : public FSMMenuItem {

public:
    MI_ACTION_LOAD_NFC()
        : FSMMenuItem(Phase::action_select, FSMResponseVariant::make(NetworkSetupResponse::scan_nfc), _("Setup via NFC")) {}
};
#endif

class MI_ACTION_SCAN : public FSMMenuItem {

public:
    MI_ACTION_SCAN()
        : FSMMenuItem(Phase::action_select, FSMResponseVariant::make(NetworkSetupResponse::scan_wifi), _("Scan networks")) {}
};

class MI_ACTION_MANUAL : public IWindowMenuItem {

public:
    MI_ACTION_MANUAL()
        : IWindowMenuItem(_("Enter credentials manually")) {}

protected:
    virtual void click(IWindowMenu &) {
        std::array<char, config_store_ns::wifi_max_ssid_len + 1> ssid = config_store().wifi_ap_ssid.get();
        if (!DialogTextInput::exec(_("SSID"), ssid)) {
            return;
        }

        std::array<char, config_store_ns::wifi_max_passwd_len + 1> password = { 0 };

        if (!DialogTextInput::exec(_(text_password), password)) {
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
    ~FrameActionSelect() {
        static_cast<window_frame_t *>(menu.GetParent())->ReleaseCaptureOfNormalWindow();
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
        SetLabel(string_view_utf8::MakeCPUFLASH(ssid_.data()));
    }

    inline int wifi_index() const {
        return wifi_index_;
    }

protected:
    virtual void click(IWindowMenu &) {
        std::array<char, config_store_ns::wifi_max_passwd_len + 1> password = { 0 };

        if (needs_password_ && !DialogTextInput::exec(_(text_password), password)) {
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

class WindowMenuWifiScan : public WindowMenuVirtual<MI_SCAN_RETURN, MI_WIFI, WindowMenuItem> {

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
        if (std::holds_alternative<MI_WIFI>(variant) && std::get<MI_WIFI>(variant).wifi_index() == wifi_index) {
            return;
        }

        std::array<char, config_store_ns::wifi_max_ssid_len + 1> ssid;
        bool needs_password;
        const auto ap_info_result = espif::scan::get_ap_info(wifi_index, std::bit_cast<std::span<uint8_t>>(std::span<const char>(ssid)), needs_password);

        auto &item = variant.emplace<MI_WIFI>(wifi_index, ssid.data(), needs_password);

        if (ap_info_result != ERR_OK) {
            item.SetLabel(string_view_utf8::MakeCPUFLASH("##ERROR##"));
            item.set_is_enabled(false);
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
                item->SetIconId(img::spinner_16x16_stages[(current_ms / 256) % img::spinner_16x16_stages.size()]);
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
    FrameWifiScan(window_t *parent)
        : menu(parent, parent->GetRect()) {

        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(menu);
    }
    ~FrameWifiScan() {
        static_cast<window_frame_t *>(menu.GetParent())->ReleaseCaptureOfNormalWindow();
    }

private:
    WindowExtendedMenu<WindowMenuWifiScan> menu;
};

class FrameText {

public:
    FrameText(window_t *parent, Phase phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info)
        : title(parent, {}, is_multiline::yes, is_closed_on_click_t::no, txt_title)
        , info(parent, {}, is_multiline::yes, is_closed_on_click_t::no, txt_info)
        , radio(parent, {}, phase) //
    {
        const auto parent_rect = parent->GetRect();
        const auto text_top = parent_rect.Top() + 64;
        const auto radio_rect = GuiDefaults::GetButtonRect(parent_rect);

        title.SetRect(Rect16::fromLTRB(0, parent_rect.Top(), parent_rect.Right(), text_top));
        title.SetAlignment(Align_t::CenterBottom());
        title.set_font(GuiDefaults::FontBig);

        info.SetRect(Rect16::fromLTRB(32, text_top + 16, parent_rect.Right() - 32, radio_rect.Bottom()));
        info.SetAlignment(Align_t::CenterTop());
#if HAS_MINI_DISPLAY()
        info.set_font(Font::small);
#endif

        radio.SetRect(radio_rect);

        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(radio);
    }

    ~FrameText() {
        static_cast<window_frame_t *>(radio.GetParent())->ReleaseCaptureOfNormalWindow();
    }

protected:
    window_text_t title;
    window_text_t info;
    RadioButtonFsm<PhaseNetworkSetup> radio;
};

class FrameAskSwitchToWifi : public FrameText {

public:
    FrameAskSwitchToWifi(window_t *parent)
        : FrameText(parent, Phase::ask_switch_to_wifi, _("Switch to Wi-Fi"), _("You're already successfully connected through the ethernet cable.\nSwitch to Wi-Fi and continue?")) {
    }
};

class FrameTextWithSSID : public FrameText {
public:
    FrameTextWithSSID(window_t *parent, Phase phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info)
        : FrameText(parent, phase, txt_title, txt_info)
        , ssid_text(parent, info.GetRect(), is_multiline::no) {
        const Rect16 r = ssid_text.GetRect();
        const auto ssid_text_height = 64;

        info.SetRect(Rect16::fromLTRB(r.Left(), r.Top() + ssid_text_height, r.Right(), r.Bottom()));

        strlcpy(ssid_buffer.data(), config_store().wifi_ap_ssid.get_c_str(), ssid_buffer.size());
        ssid_text.set_font(info.get_font());
        ssid_text.SetRect(Rect16::fromLTWH(r.Left(), r.Top(), r.Width(), ssid_text_height));
        ssid_text.SetText(string_view_utf8::MakeRAM(ssid_buffer.data()));
        ssid_text.SetAlignment(Align_t::CenterTop());
    }

protected:
    window_text_t ssid_text;
    std::array<char, config_store_ns::wifi_max_ssid_len + 1> ssid_buffer;
};

class FrameConnecting : public FrameTextWithSSID {

public:
    FrameConnecting(window_t *parent)
        : FrameTextWithSSID(parent, Phase::connecting, _("Connecting to:"), _("You can press 'Finish' to continue connecting on the background.")) {}
};

class FrameESPError : public FrameText {

public:
    FrameESPError(window_t *parent)
        : FrameText(parent, Phase::no_interface_error, _("No network interface"), _("The Wi-Fi module is not working properly or is missing.\n\nInsert the module, try restarting the printer, or use the ethernet cable.")) {
    }
};

class FrameError : public FrameText {

public:
    FrameError(window_t *parent)
        : FrameText(parent, Phase::connection_error, _("Error"), _("There was an error connecting to the Wi-Fi.")) {
    }
};

class FrameConnected : public FrameTextWithSSID {

public:
    FrameConnected(window_t *parent)
        : FrameTextWithSSID(parent, Phase::connected, _("Successfully connected to:"), _("You can now fully use all network features of the printer.")) {
        if (config_store().active_netdev.get() != NETDEV_ESP_ID) {
            ssid_text.SetText(_("Ethernet"));
        }
    }

private:
};

class FrameWaitForINI : public FrameText {

public:
    FrameWaitForINI(window_t *parent)
        : FrameText(parent, Phase::wait_for_ini_file, _("Credentials from INI"), _("Please insert a flash drive with a network configuration file.\n\nThe configuration file can be generated in PrusaSlicer.")) {
    }
};

class FrameAskDeleteINIFile : public FrameText {

public:
    FrameAskDeleteINIFile(window_t *parent)
        : FrameText(parent, Phase::ask_delete_ini_file, _("Delete INI file"), _("Delete credentials INI file? (Recommended)")) {
    }
};

class FrameHelpQR {

public:
#if PRINTER_IS_PRUSA_MINI
    static constexpr const char *qr_addr = "prusa.io/wifiminiqr";
    static constexpr const char *text_addr = "prusa.io/wifimini";

#elif PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX
    static constexpr const char *qr_addr = "prusa.io/wifimk4qr";
    static constexpr const char *text_addr = "prusa.io/wifimk4";

#elif PRINTER_IS_PRUSA_XL
    static constexpr const char *qr_addr = "prusa.io/wifixlqr";
    static constexpr const char *text_addr = "prusa.io/wifixl";

#else
    #error "not supported"
#endif

public:
    FrameHelpQR(window_t *parent)
        : text(parent, FrameQRLayout::text_rect(), is_multiline::yes)
        , link(parent, FrameQRLayout::link_rect(), is_multiline::no)
        , icon_phone(parent, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
        , qr(parent, FrameQRLayout::qrcode_rect(), qr_addr)
        , radio(parent, GuiDefaults::GetButtonRect(parent->GetRect()), Phase::help_qr) //
    {
        text.SetText(_("To setup or troubleshoot your Wi-Fi, please visit:"));
        link.SetText(string_view_utf8::MakeCPUFLASH(text_addr));

        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(radio);
    }

    ~FrameHelpQR() {
        static_cast<window_frame_t *>(radio.GetParent())->ReleaseCaptureOfNormalWindow();
    }

private:
    window_text_t text;
    window_text_t link;
    window_icon_t icon_phone;
    window_qr_t qr;
    RadioButtonFsm<PhaseNetworkSetup> radio;
};

#if HAS_NFC()
class FrameWaitForNFC : public FrameText {
    nfc::SharedEnabler nfc_enable;

public:
    FrameWaitForNFC(window_t *parent)
        : FrameText(parent, Phase::wait_for_nfc, _("Credentials via NFC"), _("1. Open Prusa app on your mobile device.\n\n2. Go to in-app Menu and select \"Set up Printer Wi-Fi.\"\n\n3. Follow on-screen instructions.")) {
    }
};

class FrameConfirmNFC : public FrameText {

public:
    FrameConfirmNFC(window_t *parent)
        : FrameText(parent, Phase::nfc_confirm, _("Credentials via NFC"), {}) {
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

using Frames = FrameDefinitionList<ScreenNetworkSetup::FrameStorage,
#if HAS_NFC()
    FrameDefinition<Phase::wait_for_nfc, FrameWaitForNFC>,
    FrameDefinition<Phase::nfc_confirm, FrameConfirmNFC>,
#endif
    FrameDefinition<Phase::ask_switch_to_wifi, FrameAskSwitchToWifi>,
    FrameDefinition<Phase::action_select, FrameActionSelect>,
    FrameDefinition<Phase::wifi_scan, FrameWifiScan>,
    FrameDefinition<Phase::ask_delete_ini_file, FrameAskDeleteINIFile>,
    FrameDefinition<Phase::wait_for_ini_file, FrameWaitForINI>,
    FrameDefinition<Phase::connecting, FrameConnecting>,
    FrameDefinition<Phase::no_interface_error, FrameESPError>,
    FrameDefinition<Phase::connection_error, FrameError>,
    FrameDefinition<Phase::help_qr, FrameHelpQR>,
    FrameDefinition<Phase::connected, FrameConnected> //
    >;

} // namespace network_wizard

using namespace network_wizard;

ScreenNetworkSetup::ScreenNetworkSetup()
    : ScreenFSM(N_("NETWORK SETUP"), GuiDefaults::RectScreenNoHeader) {
    CaptureNormalWindow(inner_frame);
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
