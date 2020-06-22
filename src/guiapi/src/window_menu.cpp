// window_menu.cpp
#include "window_menu.h" //C compatible, todo remove
#include "window_menu.hpp"
#include "gui.h"
#include "sound_C_wrapper.h"
#include "resource.h"
#include "IWindowMenuItem.hpp"

window_menu_t::window_menu_t(IWinMenuContainer *pContainer, uint8_t index)
    : IWindowMenu()
    , pContainer(pContainer) {
    color_back = gui_defaults.color_back;
    color_text = gui_defaults.color_text;
    color_disabled = gui_defaults.color_disabled;
    font = gui_defaults.font;
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

IWindowMenuItem *window_menu_t::GetItem(uint8_t index) const {
    if (!pContainer)
        return nullptr;
    if (index >= GetCount())
        return nullptr;
    return pContainer->GetItem(index);
}

IWindowMenuItem *window_menu_t::GetActiveItem() {
    return GetItem(index);
}

void window_menu_t::Increment(int dif) {
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

        if (new_index != old_index) { // optimization do not redraw when no change - still on end
            SetIndex(new_index);
            _window_invalidate((window_t *)this);
        }
    }
}

/******************************************************************************/
//non member fce

void window_menu_init(window_menu_t *window) {
    display::DrawRect(window->win.rect, window->color_back);
    gui_timer_create_txtroll(TEXT_ROLL_INITIAL_DELAY_MS, window->win.id);
}

void window_menu_done(window_menu_t *window) {
    gui_timers_delete_by_window_id(window->win.id);
}

void window_menu_set_item_index(window_t *window, int index) {
    if (window->cls->cls_id == WINDOW_CLS_MENU) {
        reinterpret_cast<window_menu_t *>(window)->SetIndex(index);
    }
}

void window_menu_draw(window_menu_t *window) {
    if (!((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        return;
    }

    const int item_height = window->font->h + window->padding.top + window->padding.bottom;
    rect_ui16_t rc_win = window->win.rect;

    const size_t visible_count = rc_win.h / item_height;
    size_t i;
    for (i = 0; i < visible_count && i < window->GetCount(); ++i) {

        IWindowMenuItem *item = window->GetItem(i + window->top_index);
        if (!item) {
            --i;
            break;
        }

        rect_ui16_t rc = { rc_win.x, uint16_t(rc_win.y + i * item_height),
            rc_win.w, uint16_t(item_height) };

        if (rect_in_rect_ui16(rc, rc_win)) {
            if (item->RollNeedInit()) {
                gui_timer_restart_txtroll(window->win.id);
                gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, window->win.id);
                item->RollInit(*window, rc);
            }
            item->Print(*window, rc);
        }
    }
    rc_win.h = rc_win.h - (i * item_height);

    if (rc_win.h) {
        rc_win.y += i * item_height;
        display::FillRect(rc_win, window->color_back);
    }
    window->win.flg &= ~WINDOW_FLG_INVALID;
}

//I think I do not need
//screen_dispatch_event
//callback should handle it
void window_menu_event(window_menu_t *window, uint8_t event, void *param) {
    IWindowMenuItem *const item = window->GetActiveItem();
    const int value = int(param);
    bool invalid = false;
    switch (event) {
    case WINDOW_EVENT_BTN_DN:

        item->Click(*window);
        //_window_invalidate((window_t *)window); //called inside click
        break;
    case WINDOW_EVENT_ENC_DN:
        if (item->IsSelected()) {
            invalid |= item->Decrement(value);
        } else {
            window->Decrement(value);
        }
        break;
    case WINDOW_EVENT_ENC_UP:
        if (item->IsSelected()) {
            invalid |= item->Increment(value);
        } else {
            window->Increment(value);
        }
        break;
    case WINDOW_EVENT_CAPT_1:
        //TODO: change flag to checked
        break;
    case WINDOW_EVENT_TIMER:
        if (!item->RollNeedInit()) {
            item->Roll(*window); //warning it is accessing gui timer
        }
        break;
    }
    if (invalid)
        _window_invalidate((window_t *)window);
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
