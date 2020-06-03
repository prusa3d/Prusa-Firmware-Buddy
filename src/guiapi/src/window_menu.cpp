// window_menu.cpp
#include "window_menu.h" //C compatible, todo remove
#include "window_menu.hpp"
#include "gui.h"
#include "cmath_ext.h"
#include "sound_C_wrapper.h"
#include "resource.h"
#include "IWindowMenuItem.hpp"
/*
void window_menu_item_spin(window_menu_t *window, int dif);
void window_menu_item_spin_fl(window_menu_t *window, int dif);
void window_menu_item_switch(window_menu_t *window);
void window_menu_item_select(window_menu_t *window, int dif);
*/

/*WindowMenuItem undefined = { "No menu_items fce!", 0, WI_LABEL | WI_DISABLED };

void window_menu_items(window_menu_t *pwindow_menu, uint16_t index,
    WindowMenuItem **ppitem, void *data) {
    *ppitem = &undefined;
}*/

window_menu_t::window_menu_t(IWinMenuContainer *pContainer, uint8_t index)
    : pContainer(pContainer) {
    color_back = gui_defaults.color_back;
    color_text = gui_defaults.color_text;
    color_disabled = gui_defaults.color_disabled;
    font = gui_defaults.font;
    //padding = gui_defaults.padding;
    //icon_rect = rect_ui16(0, 0, 16, 16);
    alignment = gui_defaults.alignment;
    setIndex(index);
    top_index = 0;
    win.flg |= WINDOW_FLG_ENABLED;
}

//private, for ctor (cannot fail)
void window_menu_t::setIndex(uint8_t new_index) {
    if (new_index && (!pContainer))
        new_index = 0;
    if (new_index >= GetCount())
        new_index = 0;
    GetItem(new_index)->SetFocus(); //set focus on new item
    index = new_index;
}

//public version of setIndex
bool window_menu_t::SetIndex(uint8_t index) {
    if (index && (!pContainer))
        return false; //cannot set non 0 without container
    if (index >= GetCount())
        return false;
    if (this->index == index)
        return true;
    GetActiveItem()->ClrFocus(); //remove focus from old item
    GetItem(index)->SetFocus();  //set focus on new item
    this->index = index;
    return true;
}

uint8_t window_menu_t::GetCount() const {
    if (!pContainer)
        return 0;
    return pContainer->GetCount();
}

IWindowMenuItem *window_menu_t::GetItem(uint8_t index) {
    if (!pContainer)
        return NULL;
    if (index >= GetCount())
        return NULL;
    return pContainer->GetItem(index);
}

IWindowMenuItem *window_menu_t::GetActiveItem() {
    return GetItem(index);
}

void window_menu_t::Incement(int dif) {
    IWindowMenuItem *item = GetActiveItem();
    if (item->IsSelected()) {
        if (item->Change(dif)) {
            _window_invalidate((window_t *)this);
        }
    } else {
        //all items can be in label mode
        int item_height = font->h + padding.top + padding.bottom;
        int visible_count = win.rect.h / item_height;
        int old_index = GetIndex();
        int new_index = old_index + dif;
        // play sound at first or last index of menu
        if (new_index < 0) {
            new_index = 0;
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
        if (new_index >= GetCount()) {
            new_index = GetCount() - 1;
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }

        if (new_index < top_index)
            top_index = new_index;
        if (new_index >= (top_index + visible_count))
            top_index = new_index - visible_count + 1;

        if (new_index != old_index) { // optimalization do not redraw when no change - still on end
            SetIndex(new_index);
            _window_invalidate((window_t *)this);
        }
    }
}

/******************************************************************************/
//non member fce

void window_menu_init(window_menu_t *window) {
    /*
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->color_disabled = gui_defaults.color_disabled;
    window->font = gui_defaults.font;
    window->padding = gui_defaults.padding;
    window->icon_rect = rect_ui16(0, 0, 16, 16);
    window->alignment = gui_defaults.alignment;
    window->SetIndex(0);
    window->top_index = 0;
    window->win.flg |= WINDOW_FLG_ENABLED;
    //window->pContainer = NULL;//set by screen_menu ctor
    window->roll.count = window->roll.px_cd = window->roll.progress = 0;
    window->last_index = 0;
    window->roll.phase = ROLL_SETUP;
    window->roll.setup = TXTROLL_SETUP_INIT;
    gui_timer_create_txtroll(TEXT_ROLL_INITIAL_DELAY_MS, window->win.id);
*/
}

void window_menu_done(window_menu_t *window) {
    gui_timers_delete_by_window_id(window->win.id);
}
/*
void window_menu_calculate_spin(const WI_SPIN_t *item, char *value, unsigned int value_size) {
    const char *format;

    if (item->range[WIO_STEP] < 10)
        format = "%.3f";
    else if (item->range[WIO_STEP] < 100)
        format = "%.2f";
    else if (item->range[WIO_STEP] < 1000)
        format = "%.1f";
    else
        format = "%.f";
    snprintf(value, value_size, format, item->value * 0.001);
}
*/
void window_menu_set_item_index(window_t *window, int index) {
    if (window->cls->cls_id == WINDOW_CLS_MENU) {
        reinterpret_cast<window_menu_t *>(window)->SetIndex(index);
    }
}

void _window_menu_draw_value(window_menu_t *window, const char *value,
    rect_ui16_t *p_rc, color_t color_text_option, color_t color_back) {
    rect_ui16_t vrc = {
        uint16_t(p_rc->x + p_rc->w), p_rc->y, uint16_t(window->font->w * strlen(value) + window->padding.left + window->padding.right), p_rc->h
    };
    vrc.x -= vrc.w;
    p_rc->w -= vrc.w;

    render_text_align(vrc, value, window->font,
        color_back, color_text_option, window->padding, window->alignment);
}

void window_menu_draw(window_menu_t *window) {
    if (!((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        return;
    }

    int item_height = window->font->h + window->padding.top + window->padding.bottom;
    rect_ui16_t rc_win = window->win.rect;

    size_t visible_count = rc_win.h / item_height;
    size_t i;
    for (i = 0; i < visible_count && i < window->GetCount(); ++i) {
        int idx = i + window->top_index;
        //WindowMenuItem *item;
        //window->menu_items(window, idx, &item, window->data);
        IWindowMenuItem *item = window->GetItem(idx);
        if (!item) {
            --i;
            break;
        }
        color_t color_text = window->color_text;
        color_t color_back = window->color_back;
        uint8_t swap = 0;

        rect_ui16_t rc = { rc_win.x, uint16_t(rc_win.y + i * item_height),
            rc_win.w, uint16_t(item_height) };
        padding_ui8_t padding = window->padding;

        if (rect_in_rect_ui16(rc, rc_win)) {
            item->Print(*window, rc);
            /*if (!item->IsEnabled()) {
                color_text = window->color_disabled;
            }

            if ((window->win.flg & WINDOW_FLG_FOCUSED) && (window->index == idx)) {
                color_t swp = color_text;
                color_text = color_back;
                color_back = swp;
                swap = ROPFN_SWAPBW;
            }

            color_t color_option = color_text;
            //if (window->mode && swap) {
            if (window->selected && swap) {
                color_option = COLOR_ORANGE;
            }

            switch (item->type & 0xff) //& 0xff == remove flags
            {
            case WI_SPIN:
            case WI_SPIN_FL: {
                const unsigned int text_len = 20;
                char value[text_len] = { '\0' };
                if (item->type & WI_SPIN_FL)
                    snprintf(value, text_len, item->wi_spin_fl.prt_format, (double)item->wi_spin_fl.value);
                else
                    window_menu_calculate_spin(&(item->wi_spin), value, text_len);

                _window_menu_draw_value(window, value, &rc, color_option, color_back);
            } break;
            case WI_SWITCH:
                if (swap)
                    color_option = COLOR_ORANGE;
            case WI_SELECT: {
                const char *value = ((const char **)item->data.wi_select.strings)[item->data.wi_select.index];

                _window_menu_draw_value(window, value, &rc, color_option, color_back);
            } break;
            }

            if (item->id_icon) {
                rect_ui16_t irc = { rc.x, rc.y,
                    window->icon_rect.w, window->icon_rect.h };
                rc.x += irc.w;
                rc.w -= irc.w;
                render_icon_align(irc, item->id_icon,
                    window->color_back, RENDER_FLG(ALIGN_CENTER, swap));
            } else {
                padding.left += window->icon_rect.w;
            }

            // render
            if ((window->win.flg & WINDOW_FLG_FOCUSED) && window->index == idx) {
                if (window->index != window->last_index) {
                    window->last_index = window->index;
                    window->roll.setup = TXTROLL_SETUP_INIT;
                    window->roll.phase = ROLL_SETUP;
                    gui_timer_restart_txtroll(window->win.id);
                    gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, window->win.id);
                }

                render_roll_text_align(rc,
                    item->label,
                    window->font,
                    padding,
                    window->alignment,
                    color_back,
                    color_text,
                    &window->roll);
            } else {
                render_text_align(rc, item->label, window->font,
                    color_back, color_text,
                    padding, window->alignment);
            }
        */
        }
    }
    rc_win.h = rc_win.h - (i * item_height);

    if (rc_win.h) {
        rc_win.y += i * item_height;
        display->fill_rect(rc_win, window->color_back);
    }
    window->win.flg &= ~WINDOW_FLG_INVALID;
}

//i think I do not need
//screen_dispatch_event
//callback should handle it
void window_menu_event(window_menu_t *window, uint8_t event, void *param) {
    IWindowMenuItem *const item = window->GetActiveItem();
    const int value = int(param);
    switch (event) {
    case WINDOW_EVENT_BTN_DN:

        item->Click(*window);
        //_window_invalidate((window_t *)window); //called inside click
        break;
    case WINDOW_EVENT_ENC_DN:
        if (item->IsSelected()) {
            item->Decrement(value);
        } else {
            window->Decrement(value);
        }
        break;
    case WINDOW_EVENT_ENC_UP:
        if (item->IsSelected()) {
            item->Incement(value);
        } else {
            window->Incement(value);
        }
        break;
    case WINDOW_EVENT_CAPT_1:
        //TODO: change flag to checked
        break; /*
    case WINDOW_EVENT_TIMER:
        roll_text_phasing(window->win.id, window->font, &window->roll);
        break;*/
    }
}

const window_class_menu_t window_class_menu = {
    {
        WINDOW_CLS_MENU,
        sizeof(window_menu_t),
        (window_init_t *)window_menu_init,
        (window_done_t *)window_menu_done,
        (window_draw_t *)window_menu_draw,
        (window_event_t *)window_menu_event,
    },
};
/*
void window_menu_item_spin(window_menu_t *window, int dif) {
    WindowMenuItem *item;
    window->menu_items(window, window->index, &item, window->data);

    const int32_t *range = item->data.wi_spin.range;
    int32_t old = item->data.wi_spin.value;

    if (dif > 0) {
        item->data.wi_spin.value = MIN(item->data.wi_spin.value + dif * range[WIO_STEP], range[WIO_MAX]);
    } else {
        item->data.wi_spin.value = MAX(item->data.wi_spin.value + dif * range[WIO_STEP], range[WIO_MIN]);
    }

    if (old != item->data.wi_spin.value)
        _window_invalidate((window_t *)window);
}

void window_menu_item_spin_fl(window_menu_t *window, int dif) {
    WindowMenuItem *item;
    window->menu_items(window, window->index, &item, window->data);

    const float *range = item->data.wi_spin_fl.range;
    float old = item->data.wi_spin_fl.value;

    if (dif > 0) {
        item->data.wi_spin_fl.value = MIN(item->data.wi_spin_fl.value + (float)dif * range[WIO_STEP], range[WIO_MAX]);
    } else {
        item->data.wi_spin_fl.value = MAX(item->data.wi_spin_fl.value + (float)dif * range[WIO_STEP], range[WIO_MIN]);
    }

    if (old != item->data.wi_spin_fl.value)
        _window_invalidate((window_t *)window);
}

void window_menu_item_switch(window_menu_t *window) {
    WindowMenuItem *item;
    window->menu_items(window, window->index, &item, window->data);

    const char **strings = item->data.wi_switch.strings;
    size_t size = 0;
    while (strings[size] != NULL) {
        size++;
    }
    item->data.wi_switch.index++;
    if (item->data.wi_switch.index >= size) {
        item->data.wi_switch.index = 0;
    }
}

void window_menu_item_select(window_menu_t *window, int dif) {
    WindowMenuItem *item;
    window->menu_items(window, window->index, &item, window->data);

    const char **strings = item->data.wi_select.strings;
    size_t size = 0;
    while (strings[size] != NULL) {
        size++;
    }

    if (dif > 0) {
        item->data.wi_select.index++;
        if (item->data.wi_select.index >= size) {
            item->data.wi_select.index = 0;
        }
    } else {
        item->data.wi_select.index--;
        if (item->data.wi_select.index < 0) {
            item->data.wi_select.index = size - 1;
        }
    }
    _window_invalidate((window_t *)window);
}*/
