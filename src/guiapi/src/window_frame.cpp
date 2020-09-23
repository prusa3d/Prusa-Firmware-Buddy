// window_frame.cpp
#include "window_frame.hpp"
#include "sound.hpp"
#include "ScreenHandler.hpp"

window_frame_t::window_frame_t(window_t *parent, Rect16 rect, is_dialog_t dialog, is_closed_on_timeout_t timeout, is_closed_on_serial_t serial)
    : window_t(parent, rect, dialog)
    , first(nullptr)
    , last(nullptr) {

    flag_timeout_close = timeout;
    flag_serial_close = serial;

    Enable();
    color_back = COLOR_BLACK;
}

void window_frame_t::SetMenuTimeoutClose() { flag_timeout_close = is_closed_on_timeout_t::yes; }
void window_frame_t::ClrMenuTimeoutClose() { flag_timeout_close = is_closed_on_timeout_t::no; }

void window_frame_t::SetOnSerialClose() { flag_serial_close = is_closed_on_serial_t::yes; }
void window_frame_t::ClrOnSerialClose() { flag_serial_close = is_closed_on_serial_t::no; }

//register sub win
void window_frame_t::RegisterSubWin(window_t *win) {
    //window must fit inside frame
    if (!rect.Contain(win->rect))
        return;

    //adding first window
    if (!(first && last)) {
        first = last = win;
        return;
    }

    window_t *pWin = first;
    while (pWin) {
        if (win->rect.HasIntersection(pWin->rect)) {
            if (win->IsDialog()) {
                pWin->HideBehindDialog();
            } else {
// error 2 windows cannot have intersection
// win is not dialog, it is guaranted there is no dialog inside this frame yet
// so no need to check it
#ifdef _DEBUG
                win->SetBackColor(COLOR_RED_ALERT);
#endif //_DEBUG
            }
        }
        pWin = pWin->GetNext();
    }

    last->SetNext(win);
    last = last->GetNext();
}

//unregister sub win
void window_frame_t::UnregisterSubWin(window_t *win) {
    window_t *prev = GetPrevSubWin(win);
    if (prev) {
        prev->SetNext(win->GetNext());
        if (win == last)
            last = prev;
    }

    //show all windows
    //todo check shadowed by dialog flag
    window_t *pWin = first;
    while (pWin) {
        pWin->ShowAfterDialog();
        pWin = pWin->GetNext();
    }

    if (last->IsDialog()) {
        // there is one or more dialogs, have to find them all
        // and hide windows behind them

        window_t *pDialog = first;
        while (pDialog) {
            if (pDialog->IsDialog()) {  //found dialog
                window_t *pWin = first; //now check all windows registered before it
                while (pWin && pWin != last) {
                    if (pDialog->rect.HasIntersection(pWin->rect)) { // found window behind
                        pWin->HideBehindDialog();                    // hide it
                    }
                    pWin = pWin->GetNext();
                }
            }
            pDialog = pDialog->GetNext();
        }
    } else {
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
    bool setChildrenInvalid = false;

    if (IsInvalid()) {
        unconditionalDraw();
        Validate();
        setChildrenInvalid = true;
    }

    window_t *ptr = first;
    while (ptr) {
        if (setChildrenInvalid) {
            //if hidden window has no intersection with other windows, it must be drawn (back color)
            if (ptr->IsVisible() || !GetFirstEnabledSubWin(ptr->rect)) {
                ptr->Invalidate();
            } else {
                ptr->Validate();
            }
        }

        ptr->Draw();
        ptr = ptr->GetNext();
    }
}

void window_frame_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    int dif = (int)param;
    window_t *pWin = GetFocusedWindow();

    switch (event) {
    case GUI_event_t::CLICK:
        if (pWin) {
            pWin->WindowEvent(this, GUI_event_t::CLICK, nullptr);
            //pWin->SetCapture(); //item must do this - only some of them
        }
        break;
    case GUI_event_t::ENC_DN:
        while (pWin && dif--) {
            window_t *const pPrev = GetPrevEnabledSubWin(pWin);
            if (!pPrev) {
                Sound_Play(eSOUND_TYPE::BlindAlert);
                break;
            } else {
                Sound_Play(eSOUND_TYPE::EncoderMove);
            }
            pWin = pPrev;
        }
        if (pWin) {
            pWin->SetFocus();
        }
        break;
    case GUI_event_t::ENC_UP:
        while (pWin && dif--) {
            window_t *const pNext = GetNextEnabledSubWin(pWin);
            if (!pNext) {
                Sound_Play(eSOUND_TYPE::BlindAlert);
                break;
            } else {
                Sound_Play(eSOUND_TYPE::EncoderMove);
            }
            pWin = pNext;
        }
        if (pWin) {
            pWin->SetFocus();
        }
        break;
    case GUI_event_t::CAPT_0:
        break;
    case GUI_event_t::CAPT_1:
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

//resend event to all children
void window_frame_t::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    window_t *ptr = first;
    while (ptr) {
        ptr->ScreenEvent(sender, event, param);
        ptr = ptr->GetNext();
    }
    windowEvent(this, event, param);
}

//resend invalidation to all children
void window_frame_t::invalidate(Rect16 validation_rect) {
    window_t *ptr = first;
    while (ptr) {
        ptr->Invalidate(validation_rect);
        ptr = ptr->GetNext();
    }
}

//resend validate to all children
void window_frame_t::validate(Rect16 validation_rect) {
    window_t *ptr = first;
    while (ptr) {
        ptr->Validate(validation_rect);
        ptr = ptr->GetNext();
    }
}

bool window_frame_t::IsChildCaptured() {
    return GetCapturedWindow()->GetParent() == this;
}

bool window_frame_t::IsChildFocused() {
    return GetFocusedWindow()->GetParent() == this;
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
    while (tmpWin && GetNextSubWin(tmpWin) != win) {
        tmpWin = GetNextSubWin(tmpWin);
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

window_t *window_frame_t::GetNextSubWin(window_t *win, Rect16 intersection_rect) const {
    if (!win)
        return nullptr;
    if (win->GetParent() != this)
        return nullptr;

    //endless loop is safe here, last window points to nullptr
    while (true) {
        win = win->GetNext();
        if (!win || win->rect.HasIntersection(intersection_rect)) {
            return win;
        }
    }
}

window_t *window_frame_t::GetPrevSubWin(window_t *win, Rect16 intersection_rect) const {
    if (!win)
        return nullptr;
    if (win->GetParent() != this)
        return nullptr;
    window_t *tmpWin = first;
    while (tmpWin && GetNextSubWin(tmpWin, intersection_rect) != win) {
        tmpWin = GetNextSubWin(tmpWin, intersection_rect);
    }
    return tmpWin;
}

window_t *window_frame_t::GetNextEnabledSubWin(window_t *win, Rect16 intersection_rect) const {
    if (!win)
        return nullptr;
    if (win->GetParent() != this)
        return nullptr;

    //endless loop is safe here, last window points to nullptr
    while (true) {
        win = win->GetNextEnabled();
        if (!win || win->rect.HasIntersection(intersection_rect)) {
            return win;
        }
    }
}

window_t *window_frame_t::GetPrevEnabledSubWin(window_t *win, Rect16 intersection_rect) const {
    window_t *tmpWin = GetPrevSubWin(win, intersection_rect);
    while (tmpWin && !tmpWin->IsEnabled()) {
        tmpWin = GetPrevSubWin(tmpWin, intersection_rect);
    }
    return tmpWin;
}

window_t *window_frame_t::GetFirstEnabledSubWin(Rect16 intersection_rect) const {
    if (!first)
        return nullptr;
    if (first->IsEnabled() && first->rect.HasIntersection(intersection_rect))
        return first;
    return GetNextEnabledSubWin(first, intersection_rect);
}

Rect16 window_frame_t::GenerateRect(ShiftDir_t dir) {
    if (!last)
        return Rect16();
    return Rect16(last->rect, dir);
}
