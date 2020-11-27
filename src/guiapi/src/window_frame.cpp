// window_frame.cpp
#include "window_frame.hpp"
#include "sound.hpp"

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
    while (((temp = findFirst(temp, end, filter)) != nullptr) && (temp != end)) {
        ret = temp;
        temp = temp->GetNext();
    };
    return ret;
}

//register sub win
//structure: popups - than normal windows - than dialogs - than strong dialogs
bool window_frame_t::registerSubWin(window_t &win) {
    //window must fit inside frame
    if (!rect.Contain(win.rect))
        return false;

    //adding first window - it must be normal window
    if (!(first && last)) {
        if (win.GetType() == win_type_t::normal) {
            first = last = &win;
            return true;
        } else {
            return false;
        }
    }

    WinFilterNormal filter_normal;
    WinFilterDialogNonStrong filter_dialog;
    WinFilterPopUp filter_popup;

    window_t *last_popup = findLast(first, nullptr, filter_popup);
    window_t *last_normal = findLast(first, nullptr, filter_normal);
    window_t *last_dialog = findLast(first, nullptr, filter_dialog);
    window_t *last_strong = last; // no need for findLast(first, nullptr, filter_strong);

    window_t *first_normal = last_popup ? last_popup->GetNext() : first;

    if (!first_normal || !last_normal)
        return false; //should not happen, there is always at least one normal window
    if (!last_dialog)
        last_dialog = last_normal;
    if (!last_strong)
        last_strong = last_dialog;

    window_t *predecessor = nullptr;

    //find place to register
    switch (win.GetType()) {
    case win_type_t::normal:
#ifdef _DEBUG
        colorConflictBackgroundToRed(win);
#endif //_DEBUG
        predecessor = last_normal;
        break;
    case win_type_t::dialog:
        predecessor = last_dialog;
        break;
    case win_type_t::popup:
        if (!canRegisterPopup(win, *first_normal, *last_strong)) {
            return false;
        }
        break;
    case win_type_t::strong_dialog:
        predecessor = last_strong;
        break;
    default:
        return false;
    }

    unregisterConflictingPopUps(win.rect, *first_normal); //normally could break predecessor of poppup, so it is found after

    if (!predecessor) {                                       // popup registration sets predecessor to nullptr
        predecessor = findLast(first, nullptr, filter_popup); // need to find it again, unregisterConflictingPopUps could break it
    }

    if (predecessor) {
        window_t *successor = predecessor->GetNext();
        registerSubWin(win, *predecessor, successor);
    } else {
        // popup registration, but no popup is registered yet
        win.SetNext(first);
        first = &win;
    }

    win.SetParent(this);
    win.Invalidate();

    clearAllHiddenBehindDialogFlags();
    hideSubwinsBehindDialogs();
    return true;
}

void window_frame_t::unregisterConflictingPopUps(Rect16 rect, window_t &first_normal) {

    WinFilterIntersectingPopUp filter_popup(rect);
    window_t *popup;
    //find intersecting popups and close them
    while ((popup = findFirst(first, &first_normal, filter_popup)) != &first_normal) {
        UnregisterSubWin(popup);
    }
}

void window_frame_t::registerSubWin(window_t &win, window_t &predecessor, window_t *pSuccessor) {
    predecessor.SetNext(&win);
    win.SetNext(pSuccessor);

    if (last == &predecessor)
        last = &win;
}

void window_frame_t::colorConflictBackgroundToRed(window_t &win) {
    //in debug windows with intersection are painted with red background
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
}

void window_frame_t::clearAllHiddenBehindDialogFlags() {
    window_t *ptr = first;
    while (ptr) {
        ptr->ShowAfterDialog();
        ptr = ptr->GetNext();
    }
}

void window_frame_t::hideSubwinsBehindDialogs() {
    WinFilterNormal filter_normal;
    WinFilterNormal filter_popup;
    window_t *last_normal = findLast(first, nullptr, filter_normal);
    window_t *last_popup = findLast(first, nullptr, filter_popup);
    window_t *first_dialog = last_normal ? last_normal->GetNext() : nullptr; //first not normal window
    window_t *first_popup = first;
    window_t *first_normal = (last_popup && last_popup->GetNext()) ? last_popup->GetNext() : first; //when last_popup != nullptr, last_popup->GetNext() != nullptr, check to be sure

    if (!last_normal)
        return; //should never happen

    // popups are in front normal windows, have to call hiding method twice
    // popups are never hidden - they must be closed before this method is called
    // first call - hide all windows coliding with popups
    if (last_popup) {
        hideSubwinsBehindDialogs(*first_normal, nullptr, *first_popup, first_normal);
    }

    // second call - hide all windows (but popups) coliding with any kind of dialog (but popup)
    if (first_dialog) {
        hideSubwinsBehindDialogs(*first_normal, nullptr, *first_dialog, nullptr);
    }
}

void window_frame_t::hideSubwinsBehindDialogs(window_t &beginNormal, window_t *pEndNormal, window_t &beginAbnormal, window_t *pEndAbnormal) {
    //find last visible dialog
    WinFilterVisible filter_visible;
    window_t *pLastVisibleDialog;
    while ((pLastVisibleDialog = findLast(&beginAbnormal, pEndAbnormal, filter_visible)) != pEndAbnormal) {
        //hide all conflicting windows
        WinFilterIntersectingVisible filter_intersecting(pLastVisibleDialog->rect);
        window_t *pIntersectingWin;
        while ((pIntersectingWin = findFirst(&beginNormal, pEndNormal, filter_intersecting)) != pEndNormal) {
            pIntersectingWin->HideBehindDialog();
        }

        pEndAbnormal = pLastVisibleDialog; //new end of search
    }
}

bool window_frame_t::HasDialogOrPopup() {
    WinFilterDialogOrPopUp filter;
    return findFirst(first, nullptr, filter) != nullptr;
}

bool window_frame_t::canRegisterPopup(window_t &win, window_t &first_normal, window_t &last_strong) {
    WinFilterIntersectingDialog filter(win.rect);
    //find intersecting non popup
    if (findFirst(&first_normal, last_strong.GetNext(), filter)) {
        //registration failed
        win.SetParent(nullptr);
        return false;
    }
    return true;
}

//unregister sub win
void window_frame_t::unregisterSubWin(window_t &win) {
    switch (win.GetType()) {
    case win_type_t::normal:
        unregisterNormal(win);
        break;
    case win_type_t::dialog:
        unregisterDialog(win);
        break;
    case win_type_t::popup:
        unregisterPopUp(win);
        break;
    case win_type_t::strong_dialog:
        unregisterStrongDialog(win);
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
    win.SetParent(nullptr);
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
