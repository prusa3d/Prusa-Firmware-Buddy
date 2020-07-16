// window_frame.cpp
#include "window_frame.hpp"
#include "sound.hpp"
#include "ScreenHandler.hpp"

void window_frame_init(window_frame_t *window) {
    if (rect_empty_ui16(window->rect)) //use display rect curent is empty
        window->rect = rect_ui16(0, 0, display::GetW(), display::GetH());
    window->flg |= WINDOW_FLG_ENABLED | WINDOW_FLG_PARENT;
    window->color_back = COLOR_BLACK;
}

void window_frame_done(window_frame_t *window) {
}

void window_frame_draw(window_frame_t *window) {
    if (window->f_visible) {
        if (window->f_invalid) {
            rect_ui16_t rc = window->rect;
            display::FillRect(rc, window->color_back);
            window->f_invalid = 0;
            window_invalidate_children(window->id);
        }
        window_draw_children(window->id);
    }
}

void window_frame_event(window_frame_t *window, uint8_t event, void *param) {
    int16_t id;
    int dif;
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        if (window_focused_ptr && window_focused_ptr->f_tag)
            Screens::Access()->DispatchEvent(window_focused_ptr, WINDOW_EVENT_CLICK, (void *)(int)window_focused_ptr->f_tag);
        if (window_ptr(window_focused()))
            window_ptr(window_focused())->SetCapture();
        break;
    case WINDOW_EVENT_ENC_DN:
        dif = (int)param;
        id = window_focused();
        while (dif--) {
            id = window_prev_enabled(id);
        }
        if (id >= 0) {
            window_t *pWin = window_ptr(id);
            if (pWin)
                pWin->SetFocus();
        } else {
            // End indicator of the frames list ->
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
        break;
    case WINDOW_EVENT_ENC_UP:
        dif = (int)param;
        id = window_focused();
        while (dif--) {
            id = window_next_enabled(id);
        }
        if (id >= 0) {
            window_t *pWin = window_ptr(id);
            if (pWin)
                pWin->SetFocus();
        } else {
            // Start indicator of the frames list <-
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
        break;
    case WINDOW_EVENT_CAPT_0:
        break;
    case WINDOW_EVENT_CAPT_1:
        if (window_ptr(window_focused())->id_parent != window->id) {
            id = window_first_child(0);
            if (window_ptr(id) ? !window_ptr(id)->IsEnabled() : true)
                id = window_next_enabled(id);
            window_t *pWin = window_ptr(id);
            if (pWin)
                pWin->SetFocus();
        }
        break;
    }
}

const window_class_frame_t window_class_frame = {
    {
        WINDOW_CLS_FRAME,
        sizeof(window_frame_t),
        (window_init_t *)window_frame_init,
        (window_done_t *)window_frame_done,
        (window_draw_t *)window_frame_draw,
        (window_event_t *)window_frame_event,
    },
};

window_frame_t::window_frame_t(window_t *first, window_t *parent, window_t *prev, rect_ui16_t rect)
    : window_t(parent, prev, rect)
    , first(first) {

    flg |= WINDOW_FLG_ENABLED | WINDOW_FLG_PARENT;
    color_back = COLOR_BLACK;
}

void window_frame_t::SetFirst(window_t *fir) {
    first = fir;
}

void window_frame_t::unconditionalDraw() {
    if (!f_visible)
        return;
    bool setChildernInvalid = false;

    if (f_invalid) {
        draw();
        f_invalid = 0;
        setChildernInvalid = true;
    }

    window_t *ptr = first;
    while (ptr) {
        if (setChildernInvalid)
            ptr->Invalidate();
        ptr->Draw();
        ptr = ptr->GetNext();
    }
}

int window_frame_t::event(window_t *sender, uint8_t event, void *param) {
    int dif = (int)param;
    window_t *pWin = window_focused_ptr;

    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        if (window_focused_ptr) {
            window_focused_ptr->Event(this, WINDOW_EVENT_CLICK, (void *)(int)window_focused_ptr->f_tag);
            //Screens::Access()->DispatchEvent(window_focused_ptr, WINDOW_EVENT_CLICK, (void *)(int)window_focused_ptr->f_tag);
            window_focused_ptr->SetCapture();
        }
        break;
    case WINDOW_EVENT_ENC_DN:
        while (pWin && dif--) {
            window_t *const pPrev = GetPrevEnabledSubWin(pWin);
            if (pPrev) {
                pWin = pPrev;
            } else {
                break;
            }
        }
        if (pWin)
            pWin->SetFocus();
        if (dif) {
            // End indicator of the frames list ->
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
        break;
    case WINDOW_EVENT_ENC_UP:
        while (pWin && dif--) {
            window_t *const pNext = GetNextEnabledSubWin(pWin);
            if (pNext) {
                pWin = pNext;
            } else {
                break;
            }
        }
        if (pWin)
            pWin->SetFocus();
        if (dif) {
            // End indicator of the frames list ->
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
        break;
    case WINDOW_EVENT_CAPT_0:
        break;
    case WINDOW_EVENT_CAPT_1:
        if (window_focused_ptr->GetParent() != this) {
            pWin = first;
            if (pWin && !pWin->IsEnabled())
                pWin = pWin->GetNextEnabled();
            if (pWin)
                pWin->SetFocus();
        }
        break;
    }
    return 0;
}

//resend event to all childern
void window_frame_t::dispatchEvent(window_t *sender, uint8_t ev, void *param) {
    window_t *ptr = first;
    while (ptr) {
        ptr->DispatchEvent(sender, ev, param);
        ptr = ptr->GetNext();
    }
    event(this, ev, param);
}

window_t *window_frame_t::GetNextSubWin(window_t *win) const {
    if (!win)
        return nullptr;
    if (win->GetParent() != this)
        return nullptr;
    return win->GetNext();
}

window_t *window_frame_t::GetPrevSubWin(window_t *win) const {
    if (!win)
        return nullptr;
    if (win->GetParent() != this)
        return nullptr;
    window_t *tmpWin = first;
    while (tmpWin && tmpWin->GetNext() != win) {
        tmpWin = tmpWin->GetNext();
    }
    return tmpWin;
}

window_t *window_frame_t::GetNextEnabledSubWin(window_t *win) const {
    if (!win)
        return nullptr;
    if (win->GetParent() != this)
        return nullptr;
    return win->GetNextEnabled();
}

window_t *window_frame_t::GetPrevEnabledSubWin(window_t *win) const {
    window_t *tmpWin = GetPrevSubWin(win);
    while (tmpWin && !tmpWin->IsEnabled()) {
        tmpWin = GetPrevSubWin(tmpWin);
    }
    return tmpWin;
}
