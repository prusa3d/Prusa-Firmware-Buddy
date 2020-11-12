// window_frame.cpp
#include "window_frame.hpp"
#include "sound.hpp"
#include "ScreenHandler.hpp"

window_frame_t::window_frame_t(window_t *parent, Rect16 rect, win_type_t type, is_closed_on_timeout_t timeout, is_closed_on_serial_t serial)
    : AddSuperWindow<window_t>(parent, rect, type)
    , first(nullptr)
    , last(nullptr) {

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
    while ((popup = findFirst(first, nullptr, filter)) != nullptr) {
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
    while (true) {
        temp = findFirst(temp, end, filter);
        if (temp != end) {
            ret = temp;
        } else {
            break;
        }
    };
    return ret;
}

//register sub win
bool window_frame_t::RegisterSubWin(window_t *win) {
    //invalid win pointer
    if (!win)
        return false;

    //different parent
    if (win->GetParent() != nullptr && win->GetParent() != this)
        return false;

    //window must fit inside frame
    if (!rect.Contain(win->rect))
        return false;

    //adding first window is always fine
    if (!(first && last)) {
        first = last = win;
        return true;
    }
    bool ret = false;
    switch (win->GetType()) {
    case win_type_t::normal:
        ret = registerNormal(*win);
        break;
    case win_type_t::dialog:
        ret = registerDialog(*win);
        break;
    case win_type_t::popup:
        ret = registerPopUp(*win); //can clear parent
        break;
    case win_type_t::strong_dialog:
        ret = registerStrongDialog(*win);
        break;
    }

    win->SetParent(ret ? this : nullptr);
    win->Invalidate();
    return ret;
}

bool window_frame_t::registerNormal(window_t &win) {

//in debug windows with intersection are painted with red background
#ifdef _DEBUG
    // check of win_type_t::normal is needed, because other registration methods can recall this one
    if (win.GetType() == win_type_t::normal) {

        window_t *pWin = first;
        while (pWin) {
            if (win.rect.HasIntersection(pWin->rect)) {
                win.SetBackColor(COLOR_RED_ALERT);
            }
            pWin = pWin->GetNext();
        }
    }
#endif //_DEBUG

    // cannot be in RegisterSubWin directly
    // if registering popup - it needs to know if can be created first
    // so it is here to be in 1 place
    WinFilterIntersectingPopUp filter(win.rect);
    window_t *popup;
    while ((popup = findFirst(first, nullptr, filter)) != nullptr) {
        UnregisterSubWin(popup);
    }

    last->SetNext(&win);
    last = last->GetNext();
    return true;
}

bool window_frame_t::registerDialog(window_t &win) {

    // todo missing implementation
    // find strong dialogs, if exists
    // register under it
    window_t *pWin = first;
    while (pWin) {
        if (win.rect.HasIntersection(pWin->rect)) {
            pWin->HideBehindDialog();
        }
        pWin = pWin->GetNext();
    }

    return registerNormal(win);
}

bool window_frame_t::registerStrongDialog(window_t &win) {
    return registerNormal(win);
}

bool window_frame_t::registerPopUp(window_t &win) {

    WinFilterIntersectingDialog filter(win.rect);
    // there can be no overlaping dialog
    if (findFirst(first, nullptr, filter) == nullptr) {
        registerDialog(win); // register like dialog
        return true;
    }
    return false;
}

//unregister sub win
void window_frame_t::UnregisterSubWin(window_t *win) {
    //is not subwin
    if ((!win) || (win->GetParent() != this)) {
        return;
    }

    switch (win->GetType()) {
    case win_type_t::normal:
        unregisterNormal(*win);
        break;
    case win_type_t::dialog:
        unregisterDialog(*win);
        break;
    case win_type_t::popup:
        unregisterPopUp(*win);
        break;
    case win_type_t::strong_dialog:
        unregisterStrongDialog(*win);
        break;
    }
}

void window_frame_t::unregisterNormal(window_t &win) {
    window_t *prev = GetPrevSubWin(&win);
    if (prev) {
        prev->SetNext(win.GetNext());
        if (last == &win)
            last = prev;
    } else {
        first = win.GetNext();
    }
}

void window_frame_t::unregisterDialog(window_t &win) {
    unregisterNormal(win);

    //show all windows
    //todo check shadowed by dialog flag
    window_t *pWin = first;
    while (pWin) {
        pWin->ShowAfterDialog();
        pWin = pWin->GetNext();
    }

    if (!first)
        return;
    if (!last)
        return; //just to be safe, if first == nullptr than last should be nullptr too

    if (last->GetType() != win_type_t::normal) {
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

void window_frame_t::unregisterStrongDialog(window_t &win) {
    unregisterDialog(win);
}

void window_frame_t::unregisterPopUp(window_t &win) {
    unregisterDialog(win);
    win.SetParent(nullptr);
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
            pWin = first;
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
    window_t *ptr = first;
    while (ptr) {
        ptr->ScreenEvent(sender, event, param);
        ptr = ptr->GetNext();
    }
    WindowEvent(this, event, param);
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
    return GetCapturedWindow() != nullptr ? (GetCapturedWindow()->GetParent() == this) : false;
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

Rect16 window_frame_t::GenerateRect(ShiftDir_t direction) {
    if (!last)
        return Rect16();
    return Rect16(last->rect, direction);
}

void window_frame_t::Shift(ShiftDir_t direction, uint16_t distance) {
    window_t *pWin = first;
    while (pWin) {
        pWin->Shift(direction, distance);
        pWin = GetNextSubWin(pWin);
    }

    super::Shift(direction, distance);
}
