// window_frame.cpp
#include "window_frame.hpp"
#include "gui.hpp"
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

window_frame_t::window_frame_t()
    //: window_t(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, display::GetW(), display::GetH()))
    : window_t(nullptr, nullptr, rect_ui16(0, 0, display::GetW(), display::GetH()))
    , first(nullptr) {

    flg |= WINDOW_FLG_ENABLED | WINDOW_FLG_PARENT;
    color_back = COLOR_BLACK;
}

void window_frame_t::SetFirst(window_t *fir) {
    first = fir;
}

void window_frame_t::Draw() {
    //window_frame_draw(this);
    if (!f_visible)
        return;
    bool setChildernInvalid = false;

    if (f_invalid) {
        display::FillRect(rect, color_back);
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

int window_frame_t::Event(window_t *sender, uint8_t event, void *param) {
    window_frame_event(this, event, param);
    return 0;
}
