// window_list.c
#include "window_list.hpp"
#include "gui.hpp"
#include "sound.hpp"

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

void window_list_init(window_list_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->padding = gui_defaults.padding;
    window->icon_rect = rect_ui16(0, 0, 16, 16);
    window->alignment = gui_defaults.alignment;
    window->count = 0;
    window->index = 0;
    window->top_index = 0;
    window->list_item = window_list_item;
    window->flg |= WINDOW_FLG_ENABLED;
}

void window_list_done(window_list_t *window) {
}

void window_list_draw(window_list_t *window) {
    if (!((window->flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))
            == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        return;
    }

    int item_height = window->font->h + window->padding.top + window->padding.bottom;
    rect_ui16_t rc_win = window->rect;

    int visible_count = rc_win.h / item_height;
    int i;
    for (i = 0; i < visible_count && i < window->count; i++) {
        int idx = i + window->top_index;
        const char *label;
        uint16_t id_icon;
        window->list_item(window, idx, &label, &id_icon);

        color_t color_text = window->color_text;
        color_t color_back = window->color_back;
        uint8_t swap = 0;

        rect_ui16_t rc = { rc_win.x, uint16_t(rc_win.y + i * item_height),
            rc_win.w, uint16_t(item_height) };
        padding_ui8_t padding = window->padding;

        if (rect_in_rect_ui16(rc, rc_win)) {
            if (window->index == idx) {
                color_t swp = color_text;
                color_text = color_back;
                color_back = swp;
                swap = ROPFN_SWAPBW;
            }

            if (id_icon) {
                rect_ui16_t irc = { rc.x, rc.y,
                    window->icon_rect.w, window->icon_rect.h };
                rc.x += irc.w;
                rc.w -= irc.w;
                render_icon_align(irc, id_icon,
                    window->color_back, RENDER_FLG(ALIGN_CENTER, swap));
            } else {
                padding.left += window->icon_rect.w;
            }

            // render
            // this MakeRAM is safe - render_text finishes its work and the local string label[] is then no longer needed
            render_text_align(rc, string_view_utf8::MakeRAM((const uint8_t *)label), window->font,
                color_back, color_text,
                padding, window->alignment);
        }
    }
    rc_win.h = rc_win.h - (i * item_height);

    if (rc_win.h) {
        rc_win.y += i * item_height;
        display::FillRect(rc_win, window->color_back);
    }
}

void window_list_event(window_list_t *window, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        if (window_ptr(window->id_parent))
            window_ptr(window->id_parent)->SetCapture();
        break;
    case WINDOW_EVENT_ENC_DN:
        window_list_dec(window, (int)param);
        break;
    case WINDOW_EVENT_ENC_UP:
        window_list_inc(window, (int)param);
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

const window_class_list_t window_class_list = {
    {
        WINDOW_CLS_LIST,
        sizeof(window_list_t),
        (window_init_t *)window_list_init,
        (window_done_t *)window_list_done,
        (window_draw_t *)window_list_draw,
        (window_event_t *)window_list_event,
    },
};

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
