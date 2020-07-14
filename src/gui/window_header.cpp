#include "window_header.hpp"
#include "config.h"
#include "marlin_client.h"
#include "../lang/i18n.h"

#ifdef BUDDY_ENABLE_ETHERNET
    #include "wui_api.h"
#endif //BUDDY_ENABLE_ETHERNET

void window_frame_draw(window_frame_t *window);

int16_t WINDOW_CLS_HEADER = 0;

#ifdef BUDDY_ENABLE_ETHERNET
static void update_ETH_icon(window_header_t *window) {
    if (get_eth_status() == ETH_UNLINKED) {
        window->icon_off(HEADER_ICON_LAN);
    } else if (get_eth_status() == ETH_NETIF_DOWN) {
        window->icon_on(HEADER_ICON_LAN);
    } else {
        window->icon_activate(HEADER_ICON_LAN);
    }
}
#endif // BUDDY_ENABLE_ETHERNET

void window_header_init(window_header_t *window) {
    window->color_back = gui_defaults.color_back;
    window->label.SetBackColor(window->color_back);
    window->label.SetTextColor(gui_defaults.color_text);
    window->label.font = gui_defaults.font;
    window->label.padding = gui_defaults.padding;
    window->label.alignment = ALIGN_LEFT_CENTER;
    window->id_res = IDR_NULL;
    window->icons[HEADER_ICON_USB] = HEADER_ISTATE_ON;
    window->icons[HEADER_ICON_LAN] = HEADER_ISTATE_OFF;
    window->icons[HEADER_ICON_WIFI] = HEADER_ISTATE_OFF;
    window->SetText(string_view_utf8::MakeNULLSTR());

    if (marlin_vars()->media_inserted) {
        window->icons[HEADER_ICON_USB] = HEADER_ISTATE_ACTIVE;
    }
#ifdef BUDDY_ENABLE_ETHERNET
    update_ETH_icon(window);
#endif //BUDDY_ENABLE_ETHERNET

    display::FillRect(gui_defaults.header_sz, window->color_back); // clear the window before drawing
}

void window_header_done(window_header_t *window) {}

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

void window_header_t::SetIcon(int16_t id_res) {
    this->id_res = id_res;
    Invalidate();
}

void window_header_t::icon_off(header_icons_t icon) {
    if (icons[icon] != HEADER_ISTATE_OFF) {
        icons[icon] = HEADER_ISTATE_OFF;
        Invalidate();
    }
}

void window_header_t::icon_on(header_icons_t icon) {
    if (icons[icon] != HEADER_ISTATE_ON) {
        icons[icon] = HEADER_ISTATE_ON;
        Invalidate();
    }
}

void window_header_t::icon_activate(header_icons_t icon) {
    if (icons[icon] != HEADER_ISTATE_ACTIVE) {
        icons[icon] = HEADER_ISTATE_ACTIVE;
        Invalidate();
    }
}

header_states_t window_header_t::GetState(header_icons_t icon) const {
    return icons[icon];
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
#ifdef BUDDY_ENABLE_ETHERNET
    update_ETH_icon(this);
#endif //BUDDY_ENABLE_ETHERNET
    if (marlin_event_clr(MARLIN_EVT_MediaInserted)) {
        icon_activate(HEADER_ICON_USB);
        return 1;
    }
    return 0;
}

bool window_header_t::EventClr_MediaRemoved() {
    /* lwip fces only read states, invalid states by another thread never mind */
#ifdef BUDDY_ENABLE_ETHERNET
    update_ETH_icon(this);
#endif //BUDDY_ENABLE_ETHERNET
    if (marlin_event_clr(MARLIN_EVT_MediaRemoved)) {
        icon_on(HEADER_ICON_USB);
        return 1;
    }
    return 0;
}

bool window_header_t::EventClr_MediaError() {
    /* lwip fces only read states, invalid states by another thread never mind */
#ifdef BUDDY_ENABLE_ETHERNET
    update_ETH_icon(this);
#endif //BUDDY_ENABLE_ETHERNET
    if (marlin_event_clr(MARLIN_EVT_MediaError)) {
        return 1;
    }
    return 0;
}

const window_class_header_t window_class_header = {
    {
        WINDOW_CLS_USER,
        sizeof(window_header_t),
        (window_init_t *)window_header_init,
        (window_done_t *)window_header_done,
        (window_draw_t *)window_header_draw,
        0,
    },
};

window_header_t::window_header_t(window_t *parent, window_t *prev)
    : window_frame_t(parent, prev, gui_defaults.header_sz)
    , label(this, nullptr) //todo calculate rect
{
    label.alignment = ALIGN_LEFT_CENTER;
    id_res = IDR_NULL;
    icons[HEADER_ICON_USB] = HEADER_ISTATE_ON;
    icons[HEADER_ICON_LAN] = HEADER_ISTATE_OFF;
    icons[HEADER_ICON_WIFI] = HEADER_ISTATE_OFF;
    SetText(string_view_utf8::MakeNULLSTR());

    if (marlin_vars()->media_inserted) {
        icons[HEADER_ICON_USB] = HEADER_ISTATE_ACTIVE;
    }
#ifdef BUDDY_ENABLE_ETHERNET
    update_ETH_icon(this);
#endif //BUDDY_ENABLE_ETHERNET
}
