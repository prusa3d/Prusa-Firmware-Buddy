#include "window_header.hpp"
#include "config.h"
#include "marlin_client.h"
#include "../lang/i18n.h"
#include "marlin_events.h"

#ifdef BUDDY_ENABLE_ETHERNET
    #include "wui_api.h"
#endif //BUDDY_ENABLE_ETHERNET

void window_frame_draw(window_frame_t *window);

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

/*
void window_header_draw(window_header_t *window) {
    if (!((window->flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))
            == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        return;
    }

    rect_ui16_t rc = {
        uint16_t(window->rect.x + 10), window->rect.y,
        window->rect.h, window->rect.h
    };

    if (window->id_res) { // first icon
        render_icon_align(rc, window->id_res,
            window->color_back, RENDER_FLG(ALIGN_CENTER, 0));
    } else {
        display::FillRect(rc, window->color_back);
    }

    uint16_t icons_width = 10 + 36;
    rc = rect_ui16( // usb icon is showed always
        window->rect.x + window->rect.w - 10 - 34, window->rect.y,
        36, window->rect.h);
    uint8_t ropfn = (window->icons[HEADER_ICON_USB] == HEADER_ISTATE_ACTIVE) ? 0 : ROPFN_DISABLE;
    render_icon_align(rc, IDR_PNG_header_icon_usb,
        window->color_back, RENDER_FLG(ALIGN_CENTER, ropfn));

    for (int i = HEADER_ICON_USB + 1; i < HEADER_ICON_COUNT; i++) {
        if (window->icons[i] > HEADER_ISTATE_OFF) {
            icons_width += 20;
            rc = rect_ui16(
                window->rect.x + window->rect.w - icons_width,
                window->rect.y, 20, window->rect.h);
            ropfn = (window->icons[i] == HEADER_ISTATE_ACTIVE) ? 0 : ROPFN_DISABLE;
            uint16_t id_res = 0;
            switch (i) {
            case HEADER_ICON_LAN:
                id_res = IDR_PNG_header_icon_lan;
                break;
            case HEADER_ICON_WIFI:
                id_res = IDR_PNG_header_icon_wifi;
                break;
            }
            render_icon_align(rc, id_res,
                window->color_back, RENDER_FLG(ALIGN_CENTER, ropfn));
        }
    }

    rc = window->rect;
    rc.x += 10 + window->rect.h;
    rc.w -= (icons_width + 10 + window->rect.h);

    if (!window->label.GetText().isNULLSTR()) { // label
        render_text_align(rc, window->label.GetText(), window->label.font,
            window->color_back, window->label.GetTextColor(),
            window->label.padding, window->label.alignment);
    }
}
*/
void window_header_t::SetIcon(int16_t id_res) {
    icon_base.SetIdRes(id_res);
    Invalidate();
}

void window_header_t::SetText(string_view_utf8 txt) {
    label.SetText(txt);
    Invalidate();
}

void window_header_t::EventClr() {
    EventClr_MediaInserted();
    EventClr_MediaRemoved();
    EventClr_MediaError();
}

bool window_header_t::EventClr_MediaInserted() {
    /* lwip fces only read states, invalid states by another thread never mind */
    update_ETH_icon();
    if (marlin_event_clr(MARLIN_EVT_MediaInserted)) {
        USB_Activate();
        return 1;
    }
    return 0;
}

bool window_header_t::EventClr_MediaRemoved() {
    /* lwip fces only read states, invalid states by another thread never mind */
    update_ETH_icon();
    if (marlin_event_clr(MARLIN_EVT_MediaRemoved)) {
        USB_On();
        return 1;
    }
    return 0;
}

bool window_header_t::EventClr_MediaError() {
    /* lwip fces only read states, invalid states by another thread never mind */
    update_ETH_icon();
    if (marlin_event_clr(MARLIN_EVT_MediaError)) {
        return 1;
    }
    return 0;
}

static const size_t icon_usb_width = 36 + 10;
static const size_t icon_lan_width = 20 + 10;
static const size_t icons_width = icon_usb_width + icon_lan_width;

window_header_t::window_header_t(window_t *parent)
    : window_frame_t(parent, gui_defaults.header_sz)
    , icon_base(this, rect_ui16(rect.x + 10, rect.y, rect.h, rect.h), 0)
    , label(this, rect_ui16(rect.x + 10 + rect.h, rect.y, rect.w - icons_width - 10 - rect.h, rect.h))
    , icon_usb(this, rect_ui16(rect.x + rect.w - icon_usb_width, rect.y, icon_usb_width, rect.h), IDR_PNG_header_icon_usb)
    , icon_lan(this, rect_ui16(rect.x + rect.w - icons_width, rect.y, icon_lan_width, rect.h), IDR_PNG_header_icon_lan) {
    label.alignment = ALIGN_LEFT_CENTER;

    LAN_Off();
    marlin_vars()->media_inserted ? USB_Activate() : USB_On();

    update_ETH_icon();
    Disable();
}

void window_header_t::USB_Off() { icon_usb.Hide(); }
void window_header_t::USB_On() {
    icon_usb.Show();
    icon_usb.Disable();
}
void window_header_t::USB_Activate() {
    icon_usb.Show();
    icon_usb.Enable();
}
void window_header_t::LAN_Off() { icon_lan.Hide(); }
void window_header_t::LAN_On() {
    icon_lan.Show();
    icon_lan.Disable();
}
void window_header_t::LAN_Activate() {
    icon_lan.Show();
    icon_lan.Enable();
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
