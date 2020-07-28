// window_frame.cpp
#include "window_frame.hpp"
#include "sound.hpp"
#include "ScreenHandler.hpp"

window_frame_t::window_frame_t(window_t *parent, rect_ui16_t rect, is_dialog_t dialog)
    : window_t(parent, rect, dialog)
    , first(nullptr)
    , last(nullptr) {
    Enable();
    color_back = COLOR_BLACK;
}

//register sub win
void window_frame_t::RegisterSubWin(window_t *win) {
    if (first && last) {
        if (win->IsDialog()) {
            window_t *pWin = first;
            while (pWin) {
                pWin->Hide(); //todo check dialog intersection
                //define shadowed by dialog flag
                pWin = pWin->GetNext();
            }
        }
        last->SetNext(win);
        last = last->GetNext();
    } else {
        first = last = win;
    }
}

//unregister sub win
void window_frame_t::UnregisterSubWin(window_t *win) {
    window_t *prev = GetPrevSubWin(win);
    if (prev) {
        prev->SetNext(win->GetNext());
        if (win == last)
            last = prev;
    }

    if (last->IsDialog()) {
        //todo check intersection with this dialog
        //and show windows that have no intersection with it
        last->Show();
    } else {
        //show all windows
        //todo check shadowed by dialog flag
        window_t *pWin = first;
        while (pWin) {
            pWin->Show();
            pWin = pWin->GetNext();
        }

        //todo remove after menu refactoring - menu items must be windows
        //needed for menu
        Invalidate();
    }
}

window_t *window_frame_t::GetFirst() const {
    return first;
}

window_t *window_frame_t::GetLast() const {
    return last;
}

void window_frame_t::draw() {
    if (!IsVisible())
        return;
    bool setChildernInvalid = false;

    if (IsInvalid()) {
        unconditionalDraw();
        Validate();
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

void window_frame_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    int dif = (int)param;
    window_t *pWin = GetFocusedWindow();

    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        if (pWin) {
            pWin->WindowEvent(this, WINDOW_EVENT_CLICK, nullptr);
            //pWin->SetCapture(); //item must do this - only some of them
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
        if (pWin->GetParent() != this) {
            pWin = first;
            if (pWin && !pWin->IsEnabled())
                pWin = pWin->GetNextEnabled();
            if (pWin)
                pWin->SetFocus();
        }
        break;
    }
}

//resend event to all childern
void window_frame_t::screenEvent(window_t *sender, uint8_t ev, void *param) {
    window_t *ptr = first;
    while (ptr) {
        ptr->ScreenEvent(sender, ev, param);
        ptr = ptr->GetNext();
    }
    windowEvent(this, ev, param);
}

//resend invalidate to all childern
void window_frame_t::invalidate(rect_ui16_t validation_rect) {
    window_t *ptr = first;
    while (ptr) {
        ptr->Invalidate(validation_rect);
        ptr = ptr->GetNext();
    }
}

//resend validate to all childern
void window_frame_t::validate(rect_ui16_t validation_rect) {
    window_t *ptr = first;
    while (ptr) {
        ptr->Validate(validation_rect);
        ptr = ptr->GetNext();
    }
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

window_t *window_frame_t::GetFirstEnabledSubWin() const {
    if (!first)
        return nullptr;
    if (first->IsEnabled())
        return first;
    return GetNextEnabledSubWin(first);
}

bool window_frame_t::IsChildCaptured() {
    return GetCapturedWindow()->GetParent() == this;
}

bool window_frame_t::IsChildFocused() {
    return GetFocusedWindow()->GetParent() == this;
}
