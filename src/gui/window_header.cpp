/*
 * window_header.c
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#include <stdbool.h>
#include "window_header.h"
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
        p_window_header_icon_off(window, HEADER_ICON_LAN);
    } else if (get_eth_status() == ETH_NETIF_DOWN) {
        p_window_header_icon_on(window, HEADER_ICON_LAN);
    } else {
        p_window_header_icon_active(window, HEADER_ICON_LAN);
    }
}
#endif // BUDDY_ENABLE_ETHERNET

void window_header_init(window_header_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->padding = gui_defaults.padding;
    window->alignment = ALIGN_LEFT_CENTER;
    window->id_res = IDR_NULL;
    window->icons[HEADER_ICON_USB] = HEADER_ISTATE_ON;
    window->icons[HEADER_ICON_LAN] = HEADER_ISTATE_OFF;
    window->icons[HEADER_ICON_WIFI] = HEADER_ISTATE_OFF;
    window->label = NULL;

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
    if (!((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))
            == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        return;
    }

    rect_ui16_t rc = {
        uint16_t(window->win.rect.x + 10), window->win.rect.y,
        window->win.rect.h, window->win.rect.h
    };

    if (window->id_res) { // first icon
        render_icon_align(rc, window->id_res,
            window->color_back, RENDER_FLG(ALIGN_CENTER, 0));
    } else {
        display::FillRect(rc, window->color_back);
    }

    uint16_t icons_width = 10 + 36;
    rc = rect_ui16( // usb icon is showed always
        window->win.rect.x + window->win.rect.w - 10 - 34, window->win.rect.y,
        36, window->win.rect.h);
    uint8_t ropfn = (window->icons[HEADER_ICON_USB] == HEADER_ISTATE_ACTIVE) ? 0 : ROPFN_DISABLE;
    render_icon_align(rc, IDR_PNG_header_icon_usb,
        window->color_back, RENDER_FLG(ALIGN_CENTER, ropfn));

    for (int i = HEADER_ICON_USB + 1; i < HEADER_ICON_COUNT; i++) {
        if (window->icons[i] > HEADER_ISTATE_OFF) {
            icons_width += 20;
            rc = rect_ui16(
                window->win.rect.x + window->win.rect.w - icons_width,
                window->win.rect.y, 20, window->win.rect.h);
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

    rc = window->win.rect;
    rc.x += 10 + window->win.rect.h;
    rc.w -= (icons_width + 10 + window->win.rect.h);

    if (window->label) {                                      // label
        render_text_align(rc, _(window->label), window->font, // @@TODO verify, that this is the right spot to translate window labels
            window->color_back, window->color_text,
            window->padding, window->alignment);
    }
}

void p_window_header_set_icon(window_header_t *window, uint16_t id_res) {
    window->id_res = id_res;
    _window_invalidate((window_t *)window);
}

void p_window_header_icon_off(window_header_t *window, header_icons_t icon) {
    if (window->icons[icon] != HEADER_ISTATE_OFF) {
        window->icons[icon] = HEADER_ISTATE_OFF;
        _window_invalidate((window_t *)window);
    }
}

void p_window_header_icon_on(window_header_t *window, header_icons_t icon) {
    if (window->icons[icon] != HEADER_ISTATE_ON) {
        window->icons[icon] = HEADER_ISTATE_ON;
        _window_invalidate((window_t *)window);
    }
}

void p_window_header_icon_active(window_header_t *window, header_icons_t icon) {
    if (window->icons[icon] != HEADER_ISTATE_ACTIVE) {
        window->icons[icon] = HEADER_ISTATE_ACTIVE;
        _window_invalidate((window_t *)window);
    }
}

header_states_t p_window_header_get_state(window_header_t *window,
    header_icons_t icon) {
    return window->icons[icon];
}

void p_window_header_set_text(window_header_t *window, const char *text) {
    window->label = text;
    _window_invalidate((window_t *)window);
}

int p_window_header_event_clr(window_header_t *window, MARLIN_EVT_t evt_id) {
    /* lwip fces only read states, invalid states by another thread never mind */
#ifdef BUDDY_ENABLE_ETHERNET
    update_ETH_icon(window);
#endif //BUDDY_ENABLE_ETHERNET
    if (marlin_event_clr(evt_id)) {
        switch (evt_id) {
        case MARLIN_EVT_MediaInserted:
            p_window_header_icon_active(window, HEADER_ICON_USB);
            break;
        case MARLIN_EVT_MediaRemoved:
            p_window_header_icon_on(window, HEADER_ICON_USB);
            break;
        default:
            break;
        }
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
