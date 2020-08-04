// window_list.c
#include "window_list.hpp"
#include "gui.hpp"
#include "sound.hpp"
#include "../lang/i18n.h"

const char items[11][6] = {
    "item0",
    "item1",
    "item2",
    "item3",
    "item4",
    "item5",
    "item6",
    "item7",
    "item8",
    "item9",
    "itemA",
};

void window_list_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    *pptext = (char *)items[index];
    *pid_icon = 0;
}

void window_list_inc(window_list_t *window, int dif);
void window_list_dec(window_list_t *window, int dif);

void window_list_t::unconditionalDraw() {

    int item_height = font->h + padding.top + padding.bottom;
    rect_ui16_t rc_win = rect;

    int visible_count = rc_win.h / item_height;
    int i;
    for (i = 0; i < visible_count && i < count; i++) {
        int idx = i + top_index;
        const char *label;
        uint16_t id_icon;
        list_item(this, idx, &label, &id_icon);

        color_t text_cl = color_text;
        color_t back_cl = color_back;
        uint8_t swap = 0;

        rect_ui16_t rc = { rc_win.x, uint16_t(rc_win.y + i * item_height),
            rc_win.w, uint16_t(item_height) };
        padding_ui8_t padd = padding;

        if (rect_in_rect_ui16(rc, rc_win)) {
            if (index == idx) {
                color_t swp = text_cl;
                text_cl = back_cl;
                back_cl = swp;
                swap = ROPFN_SWAPBW;
            }

            if (id_icon) {
                rect_ui16_t irc = { rc.x, rc.y,
                    icon_rect.w, icon_rect.h };
                rc.x += irc.w;
                rc.w -= irc.w;
                render_icon_align(irc, id_icon,
                    back_cl, RENDER_FLG(ALIGN_CENTER, swap));
            } else {
                padd.left += icon_rect.w;
            }

            // render
            render_text_align(rc, _(label), font,
                back_cl, text_cl,
                padd, alignment);
        }
    }
    rc_win.h = rc_win.h - (i * item_height);

    if (rc_win.h) {
        rc_win.y += i * item_height;
        display::FillRect(rc_win, color_back);
    }
}

void window_list_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        if (GetParent())
            GetParent()->SetCapture();
        break;
    case WINDOW_EVENT_ENC_DN:
        window_list_dec(this, (int)param);
        break;
    case WINDOW_EVENT_ENC_UP:
        window_list_inc(this, (int)param);
        break;
    case WINDOW_EVENT_CAPT_1:
        //TODO: change flag to checked
        break;
    }
}

void window_list_inc(window_list_t *window, int dif) {
    int item_height = window->font->h + window->padding.top + window->padding.bottom;
    int visible_count = window->rect.h / item_height;
    int old = window->index;
    window->index += dif;
    if (window->index < 0) {
        window->index = 0;
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    if (window->index >= window->count) {
        window->index = window->count - 1;
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }

    if (window->index < window->top_index) {
        window->top_index = window->index;
    }
    if (window->index >= (window->top_index + visible_count)) {
        window->top_index = window->index - visible_count + 1;
    }

    if (window->index != old) {
        // optimalization do not redraw when no change - still on end
        window->Invalidate();
    }
}

void window_list_dec(window_list_t *window, int dif) {
    window_list_inc(window, -dif);
}

void window_list_t::SetItemCount(int cnt) {
    count = cnt;
    Invalidate();
}

void window_list_t::SetItemIndex(int idx) {
    if (count > idx) {
        index = idx;
    }
    Invalidate();
}

void window_list_t::SetTopIndex(int idx) {
    top_index = idx;
    Invalidate();
}

void window_list_t::SetCallback(window_list_item_t *fnc) {
    list_item = fnc;
}

window_list_t::window_list_t(window_t *parent, rect_ui16_t rect)
    : window_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , padding(GuiDefaults::Padding)
    , alignment(GuiDefaults::Alignment)
    , icon_rect(rect_ui16(0, 0, 16, 16))
    , count(0)
    , index(0)
    , top_index(0)
    , list_item(window_list_item) {
    Enable();
}
