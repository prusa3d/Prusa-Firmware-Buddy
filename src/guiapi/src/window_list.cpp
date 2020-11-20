// window_list.c
#include "window_list.hpp"
#include "gui.hpp"
#include "sound.hpp"
#include "i18n.h"

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
    Rect16 rc_win = rect;

    int visible_count = rc_win.Height() / item_height;
    int i;
    for (i = 0; i < visible_count && i < count; i++) {
        int idx = i + top_index;
        const char *label;
        uint16_t id_icon;
        list_item(this, idx, &label, &id_icon);

        color_t text_cl = color_text;
        color_t back_cl = color_back;
        uint8_t swap = 0;

        Rect16 rc = { rc_win.Left(), int16_t(rc_win.Top() + i * item_height),
            rc_win.Width(), uint16_t(item_height) };
        padding_ui8_t padd = padding;

        if (rc_win.Contain(rc)) {
            if (index == idx) {
                color_t swp = text_cl;
                text_cl = back_cl;
                back_cl = swp;
                swap = ROPFN_SWAPBW;
            }

            if (id_icon) {
                Rect16 irc = { rc.Left(), rc.Top(),
                    icon_rect.Width(), icon_rect.Height() };
                rc += Rect16::Left_t(irc.Width());
                rc -= irc.Width();
                render_icon_align(irc, id_icon,
                    back_cl, RENDER_FLG(ALIGN_CENTER, swap));
            } else {
                padd.left += icon_rect.Width();
            }

            // render
            render_text_align(rc, _(label), font,
                back_cl, text_cl,
                padd, GetAlignment());
        }
    }
    rc_win -= Rect16::Height_t(i * item_height);

    if (rc_win.Height()) {
        rc_win += Rect16::Top_t(i * item_height);
        display::FillRect(rc_win, color_back);
    }
}

void window_list_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        if (GetParent())
            GetParent()->SetCapture();
        break;
    case GUI_event_t::ENC_DN:
        window_list_dec(this, (int)param);
        break;
    case GUI_event_t::ENC_UP:
        window_list_inc(this, (int)param);
        break;
    case GUI_event_t::CAPT_1:
        //TODO: change flag to checked
        break;
    default:
        break;
    }
}

void window_list_inc(window_list_t *window, int dif) {
    const int item_height = window->font->h + window->padding.top + window->padding.bottom;
    const int visible_count = window->rect.Height() / item_height;
    int old_index = window->index;
    window->index += dif;
    if (window->index < 0) {
        window->index = 0;
        Sound_Play(eSOUND_TYPE::BlindAlert);
    }
    if (window->index >= window->count) {
        window->index = window->count - 1;
        Sound_Play(eSOUND_TYPE::BlindAlert);
    }

    if (window->index < window->top_index) {
        window->top_index = window->index;
    }
    if (window->index >= (window->top_index + visible_count)) {
        window->top_index = window->index - visible_count + 1;
    }

    if (window->index != old_index) {
        // optimization - do not redraw if no change (e.g. at the end of the list)
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

window_list_t::window_list_t(window_t *parent, Rect16 rect)
    : window_aligned_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , padding(GuiDefaults::Padding)
    , icon_rect(Rect16(0, 0, 16, 16))
    , count(0)
    , index(0)
    , top_index(0)
    , list_item(window_list_item) {
    Enable();
}
