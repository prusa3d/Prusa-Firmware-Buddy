#include "window_header.hpp"
#include "config.h"
#include "marlin_client.h"
#include "i18n.h"
#include "marlin_events.h"
#include "gui_media_events.hpp"

#ifdef BUDDY_ENABLE_ETHERNET
    #include "wui_api.h"
#endif //BUDDY_ENABLE_ETHERNET

void window_header_t::update_ETH_icon() {
#ifdef BUDDY_ENABLE_ETHERNET
    if (get_eth_status() == ETH_UNLINKED) {
        LAN_Off();
    } else if (get_eth_status() == ETH_NETIF_DOWN) {
        LAN_On();
    } else {
        LAN_Activate();
    }
#else
    LAN_Off();
#endif // BUDDY_ENABLE_ETHERNET
}

void window_header_t::SetIcon(int16_t id_res) {
    icon_base.SetIdRes(id_res);
    Invalidate();
}

void window_header_t::SetText(string_view_utf8 txt) {
    label.SetText(txt);
    Invalidate();
}

static const uint16_t span = 2 + 2;
static const Rect16::Width_t icon_usb_width(36);
static const Rect16::Width_t icon_lan_width(20);
static const Rect16::Width_t icons_width(icon_usb_width + icon_lan_width);
static const Rect16::Width_t icon_base_width(40);

window_header_t::window_header_t(window_t *parent, string_view_utf8 txt)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectHeader)
    , icon_base(this, Rect16(rect.TopLeft(), icon_base_width, rect.Height() - 5), 0)
    , label(this, rect - Rect16::Width_t(icons_width + span + icon_base_width) + Rect16::Left_t(icon_base_width), txt, Align_t::LeftBottom())
    , icon_usb(this, (rect + Rect16::Left_t(rect.Width() - icon_usb_width)) = icon_usb_width, IDR_PNG_usb_16px)
    , icon_lan(this, (rect + Rect16::Left_t(rect.Width() - icons_width)) = icon_lan_width, IDR_PNG_lan_16px)
    , LAN_changed_off(false) {
    icon_base.SetAlignment(Align_t::CenterBottom());

    marlin_vars()->media_inserted ? USB_Activate() : USB_On();

    update_ETH_icon();
    Disable();
}

void window_header_t::USB_Off() { icon_usb.Hide(); }
void window_header_t::USB_On() {
    icon_usb.Show();
    icon_usb.Shadow();
}
void window_header_t::USB_Activate() {
    icon_usb.Show();
    icon_usb.Unshadow();
}
void window_header_t::LAN_Off() {
    icon_lan.Hide();
    if (!LAN_changed_off) {
        LAN_changed_off = true;
        Invalidate();
    }
}
void window_header_t::LAN_On() {
    LAN_changed_off = false;
    icon_lan.Show();
    icon_lan.Shadow();
}
void window_header_t::LAN_Activate() {
    LAN_changed_off = false;
    icon_lan.Show();
    icon_lan.Unshadow();
}

window_header_t::header_states_t window_header_t::GetStateUSB() const {
    if (!icon_usb.IsVisible())
        return header_states_t::OFF;
    return icon_usb.IsEnabled() ? header_states_t::ACTIVE : header_states_t::ON;
}
window_header_t::header_states_t window_header_t::GetStateLAN() const {
    if (!icon_lan.IsVisible())
        return header_states_t::OFF;
    return icon_lan.IsEnabled() ? header_states_t::ACTIVE : header_states_t::ON;
}

void window_header_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    if (event == GUI_event_t::MEDIA) {
        switch (GuiMediaEventsHandler::state_t(int(param))) {
        case GuiMediaEventsHandler::state_t::inserted:
            USB_Activate();
            break;
        case GuiMediaEventsHandler::state_t::removed:
            USB_On();
            break;
        case GuiMediaEventsHandler::state_t::error:
            USB_Off();
            break;
        default:
            break;
        }
    }
    if (event == GUI_event_t::LOOP) {
        update_ETH_icon();
    }

    SuperWindowEvent(sender, event, param);
}
