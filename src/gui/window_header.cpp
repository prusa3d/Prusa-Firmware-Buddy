#include "window_header.hpp"
#include "config.h"
#include "i18n.h"
#include "gui_media_events.hpp"
#include "netdev.h"

void window_header_t::updateNetwork(uint32_t netdev_id, bool force) {
    bool invalidate = false;
    uint32_t netdev_status = netdev_get_status(netdev_id);

    if (netdev_id != active_netdev_id) {
        icon_network.SetIdRes(window_header_t::networkIcon(netdev_id));
        invalidate = true;
        active_netdev_id = netdev_id;
    }

    if (active_netdev_id != NETDEV_NODEV_ID
        || active_netdev_status != netdev_status
        || force) {
        if (netdev_status == NETDEV_NETIF_DOWN || netdev_status == NETDEV_UNLINKED) {
            icon_network.Shadow();
        } else {
            icon_network.Unshadow();
        }
        active_netdev_status = netdev_status;
    }

    if (invalidate) {
        Invalidate();
    }
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
    , icon_base(this, Rect16(GetRect().TopLeft(), icon_base_width, Height() - 5), 0)
    , label(this, GetRect() - Rect16::Width_t(icons_width + span + icon_base_width) + Rect16::Left_t(icon_base_width), txt, Align_t::LeftBottom())
    , icon_usb(this, (GetRect() + Rect16::Left_t(Width() - icon_usb_width)) = icon_usb_width, IDR_PNG_usb_16px)
    , icon_network(this, (GetRect() + Rect16::Left_t(Width() - icons_width)) = icon_lan_width, window_header_t::networkIcon(netdev_get_active_id()))
    , active_netdev_id(netdev_get_active_id())
    , active_netdev_status(netdev_get_status(active_netdev_id)) {
    icon_base.SetAlignment(Align_t::CenterBottom());

    updateMedia(GuiMediaEventsHandler::Get());
    updateNetwork(active_netdev_id, true);
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

void window_header_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    if (event == GUI_event_t::MEDIA) {
        updateMedia(MediaState_t(int(param)));
    }
#ifdef BUDDY_ENABLE_ETHERNET
    if (event == GUI_event_t::LOOP) {
        updateNetwork(netdev_get_active_id());
    }
#endif
    SuperWindowEvent(sender, event, param);
}

void window_header_t::updateMedia(MediaState_t state) {
    switch (state) {
    case MediaState_t::inserted:
        USB_Activate();
        break;
    case MediaState_t::removed:
        USB_On();
        break;
    case MediaState_t::error:
    default:
        USB_Off();
        break;
    }
};

uint32_t window_header_t::networkIcon(uint32_t netdev_id) {
    uint32_t res_id = IDR_NULL;

    switch (netdev_id) {
    case NETDEV_ETH_ID:
        res_id = IDR_PNG_lan_16px;
        break;
    case NETDEV_ESP_ID:
        res_id = IDR_PNG_wifi_16px;
        break;
    default:
        break;
    }

    return res_id;
}
