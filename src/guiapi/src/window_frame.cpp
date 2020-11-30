// window_frame.cpp
#include "window_frame.hpp"
#include "sound.hpp"

window_frame_t::window_frame_t(window_t *parent, Rect16 rect, win_type_t type, is_closed_on_timeout_t timeout, is_closed_on_serial_t serial)
    : AddSuperWindow<window_t>(parent, rect, type)
    , first_normal(nullptr)
    , last_normal(nullptr) {

    flags.timeout_close = timeout;
    flags.serial_close = serial;

    Enable();
}

// popup windows are static and must be unregistred
// other windows does not need unregistration
// because dialog/strong dialog from marlin thread will consume close flag so screen cannot be closed before it
// msgbox-like dialogs exist within events and screen cannot be closed inside an event
// normal windows are already unregistered by their destructors
window_frame_t::~window_frame_t() {
    WinFilterPopUp filter;
    window_t *popup;
    while ((popup = findFirst(first_normal, nullptr, filter)) != nullptr) {
        UnregisterSubWin(popup);
    }
}

void window_frame_t::SetMenuTimeoutClose() { flags.timeout_close = is_closed_on_timeout_t::yes; }
void window_frame_t::ClrMenuTimeoutClose() { flags.timeout_close = is_closed_on_timeout_t::no; }

void window_frame_t::SetOnSerialClose() { flags.serial_close = is_closed_on_serial_t::yes; }
void window_frame_t::ClrOnSerialClose() { flags.serial_close = is_closed_on_serial_t::no; }

window_t *window_frame_t::findFirst(window_t *begin, window_t *end, const WinFilter &filter) const {
    while (begin && (begin->GetParent() == this) && (begin != end)) {
        if (filter(*begin)) {
            return begin;
        }
        begin = begin->GetNext();
    }
    return end;
}

window_t *window_frame_t::findLast(window_t *begin, window_t *end, const WinFilter &filter) const {
    //no need to check parrent or null, findFirst does that
    window_t *ret = end;
    window_t *temp = begin;
    while (((temp = findFirst(temp, end, filter)) != nullptr) && (temp != end)) {
        ret = temp;
        temp = temp->GetNext();
    };
    return ret;
}

//register sub win
//structure: popups - than normal windows - than dialogs - than strong dialogs
bool window_frame_t::registerSubWin(window_t &win) {
    // only normal windows can be registered
    // screen_t handles advanced window registration
    if (win.GetType() != win_type_t::normal)
        return false;

    registerAnySubWin(win, first_normal, last_normal);

    return true;
}

void window_frame_t::registerAnySubWin(window_t &win, window_t *&pFirst, window_t *&pLast) {
    if ((!pFirst) || (!pLast)) {
        pFirst = pLast = &win;
    } else {
        window_t *pSuccessor = pLast->GetNext();
        pLast->SetNext(&win);
        win.SetNext(pSuccessor);
        pLast = &win;
    }
    win.SetParent(this);
    win.Invalidate();
}

void window_frame_t::colorConflictBackgroundToRed(window_t &win) {
    //in debug windows with intersection are painted with red background
    // check of win_type_t::normal is needed, because other registration methods can recall this one
    if (win.GetType() == win_type_t::normal) {

        window_t *pWin = first_normal;
        while (pWin) {
            if (win.rect.HasIntersection(pWin->rect)) {
                win.SetBackColor(COLOR_RED_ALERT);
            }
            pWin = pWin->GetNext();
        }
    }
}

void window_frame_t::clearAllHiddenBehindDialogFlags() {
    window_t *ptr = first_normal;
    while (ptr) {
        ptr->ShowAfterDialog();
        ptr = ptr->GetNext();
    }
}

bool window_frame_t::HasDialogOrPopup() {
    WinFilterDialogOrPopUp filter;
    return findFirst(first_normal, nullptr, filter) != nullptr;
}

//unregister sub win
void window_frame_t::unregisterSubWin(window_t &win) {
    if (win.GetType() == win_type_t::normal)
        unregisterAnySubWin(win, first_normal, last_normal);
}

void window_frame_t::unregisterAnySubWin(window_t &win, window_t *&pFirst, window_t *&pLast) {
    if ((!pFirst) || (!pLast))
        return;

    bool clr_begin_end = (&win == pFirst && pFirst == pLast);

    window_t *prev = GetPrevSubWin(&win);
    if (prev) {
        prev->SetNext(win.GetNext());
        if (pLast == &win)
            pLast = prev;
    }

    if (pFirst == &win) {
        pFirst = win.GetNext();
    }

    if (clr_begin_end) {
        pFirst = nullptr;
        pLast = nullptr;
    }
}

window_t *window_frame_t::GetFirstNormal() const {
    return first_normal;
}

window_t *window_frame_t::GetLastNormal() const {
    return last_normal;
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

    window_t *ptr = first_normal;
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

void window_frame_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    int dif = (int)param;
    window_t *pWin = GetFocusedWindow();
    if (!pWin || !pWin->IsChildOf(this))
        pWin = nullptr;

    switch (event) {
    case GUI_event_t::CLICK:
        if (pWin) {
            pWin->WindowEvent(this, GUI_event_t::CLICK, nullptr);
            //pWin->SetCapture(); //item must do this - only some of them
        } else {
            //todo should not I resend event to super?
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
        if (pWin && pWin->GetParent() != this) {
            pWin = first_normal;
            if (pWin && !pWin->IsEnabled())
                pWin = pWin->GetNextEnabled();
            if (pWin)
                pWin->SetFocus();
        }
        break;
    default:
        break;
    }
}

//resend event to all children
void window_frame_t::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    window_t *ptr = first_normal;
    while (ptr) {
        ptr->ScreenEvent(sender, event, param);
        ptr = ptr->GetNext();
    }
    WindowEvent(this, event, param);
}

//resend invalidation to all children
void window_frame_t::invalidate(Rect16 validation_rect) {
    window_t *ptr = first_normal;
    while (ptr) {
        ptr->Invalidate(validation_rect);
        ptr = ptr->GetNext();
    }
}

//resend validate to all children
void window_frame_t::validate(Rect16 validation_rect) {
    window_t *ptr = first_normal;
    while (ptr) {
        ptr->Validate(validation_rect);
        ptr = ptr->GetNext();
    }
}

bool window_frame_t::IsChildFocused() {
    return GetFocusedWindow() != nullptr ? (GetFocusedWindow()->GetParent() == this) : false;
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
    window_t *tmpWin = first_normal;
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
    if (!first_normal)
        return nullptr;
    if (first_normal->IsEnabled())
        return first_normal;
    return GetNextEnabledSubWin(first_normal);
}

window_t *window_frame_t::GetNextSubWin(window_t *win, Rect16 intersection_rect) const {
    if (!win)
        return nullptr;
    if (win->GetParent() != this)
        return nullptr;

    //endless loop is safe here, last_normal window points to nullptr
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
    window_t *tmpWin = first_normal;
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

    //endless loop is safe here, last_normal window points to nullptr
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
    if (!first_normal)
        return nullptr;
    if (first_normal->IsEnabled() && first_normal->rect.HasIntersection(intersection_rect))
        return first_normal;
    return GetNextEnabledSubWin(first_normal, intersection_rect);
}

Rect16 window_frame_t::GenerateRect(ShiftDir_t direction) {
    if (!last_normal)
        return Rect16();
    return Rect16(last_normal->rect, direction);
}

void window_frame_t::Shift(ShiftDir_t direction, uint16_t distance) {
    window_t *pWin = first_normal;
    while (pWin) {
        pWin->Shift(direction, distance);
        pWin = GetNextSubWin(pWin);
    }

    super::Shift(direction, distance);
}
