// window_frame.cpp
#include "window_frame.hpp"
#include "gui_invalidate.hpp"
#include "sound.hpp"
#include "display.hpp"
#include "marlin_client.hpp"

window_frame_t::window_frame_t(window_t *parent, Rect16 rect, win_type_t type, is_closed_on_timeout_t timeout, is_closed_on_printing_t close_on_print)
    : window_t(parent, rect, type)
    , captured_normal_window(nullptr)
    , first_normal(nullptr)
    , last_normal(nullptr) {

    flags.timeout_close = timeout;
    flags.print_close = close_on_print;

    Enable();
}

window_frame_t::window_frame_t(window_t *parent, Rect16 rect, positioning sub_win_pos)
    : window_frame_t(parent, rect, win_type_t::normal, is_closed_on_timeout_t::yes, is_closed_on_printing_t::yes) {
    flags.has_relative_subwins = sub_win_pos == positioning::relative;
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
        UnregisterSubWin(*popup);
    }
}

void window_frame_t::SetMenuTimeoutClose() { flags.timeout_close = is_closed_on_timeout_t::yes; }
void window_frame_t::ClrMenuTimeoutClose() { flags.timeout_close = is_closed_on_timeout_t::no; }

void window_frame_t::SetOnSerialClose() { flags.print_close = is_closed_on_printing_t::yes; }
void window_frame_t::ClrOnSerialClose() { flags.print_close = is_closed_on_printing_t::no; }

window_t *window_frame_t::findFirst(window_t *begin, window_t *end, const WinFilter &filter) const {
    if (!begin) {
        return end;
    }
    window_t *parent = begin->GetParent();
    if ((parent == nullptr) || (end && (end->GetParent() != parent))) {
        return end;
    }
    while (begin && (begin->GetParent() == parent) && (begin != end)) {
        if (filter(*begin)) {
            return begin;
        }
        begin = begin->GetNext();
    }
    return end;
}

window_t *window_frame_t::findLast(window_t *begin, window_t *end, const WinFilter &filter) const {
    // no need to check parrent or null, findFirst does that
    window_t *ret = end;
    window_t *temp = begin;
    while (((temp = findFirst(temp, end, filter)) != nullptr) && (temp != end)) {
        ret = temp;
        temp = temp->GetNext();
    };
    return ret;
}

// register sub win
// structure: popups - than normal windows - than dialogs - than strong dialogs
bool window_frame_t::registerSubWin(window_t &win) {
    // only normal windows can be registered
    // screen_t handles advanced window registration
    if (win.GetType() != win_type_t::normal) {
        return false;
    }

    registerAnySubWin(win, first_normal, last_normal);

    return true;
}

void window_frame_t::registerAnySubWin(window_t &win, CompactRAMPointer<window_t> &pFirst, CompactRAMPointer<window_t> &pLast) {
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
    // in debug windows with intersection are painted with red background
    //  check of win_type_t::normal is needed, because other registration methods can recall this one
    if (win.GetType() == win_type_t::normal) {

        window_t *pWin = first_normal;
        while (pWin) {
            if (win.GetRect().HasIntersection(pWin->GetRect())) {
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

// unregister sub win
void window_frame_t::unregisterSubWin(window_t &win) {
    if (win.GetType() == win_type_t::normal) {
        unregisterAnySubWin(win, first_normal, last_normal);
    }
}

void window_frame_t::unregisterAnySubWin(window_t &win, CompactRAMPointer<window_t> &pFirst, CompactRAMPointer<window_t> &pLast) {
    if ((!pFirst) || (!pLast)) {
        return;
    }

    if (captured_normal_window == &win) {
        ReleaseCaptureOfNormalWindow();
    }

    Rect16 inv_rect = win.GetRect();
    bool clr_begin_end = (&win == pFirst && pFirst == pLast);

    window_t *prev = GetPrevSubWin(&win);
    if (prev) {
        prev->SetNext(win.GetNext());
        if (pLast == &win) {
            pLast = prev;
        }
    }

    if (pFirst == &win) {
        pFirst = win.GetNext();
    }

    if (clr_begin_end) {
        pFirst = nullptr;
        pLast = nullptr;
    }

    win.SetParent(nullptr);

    Invalidate(inv_rect);
}

void window_frame_t::addInvalidationRect(Rect16 rc) {
    invalid_area += rc;
}

Rect16 window_frame_t::getInvalidationRect() const {
    return invalid_area;
}

window_t *window_frame_t::getFirstNormal() const {
    return first_normal;
}

window_t *window_frame_t::getLastNormal() const {
    return last_normal;
}

void window_frame_t::draw() {
    if (!IsVisible()) {
        return;
    }
    bool setChildrenInvalid = false;

    if (IsInvalid()) {
        unconditionalDraw();
        setChildrenInvalid = true;
    } else {
        // invalid_area must be drawn before subwins
        if (!invalid_area.IsEmpty()) {
            display::fill_rect(invalid_area, GetBackColor());
        }
    }

    window_t *ptr = first_normal;
    while (ptr) {
        if (setChildrenInvalid) {
            // if hidden window has no intersection with other windows, it must be drawn (back color)
            if (ptr->IsVisible() || !GetFirstEnabledSubWin(ptr->GetRect())) {
                ptr->Invalidate();
            } else {
                ptr->Validate();
            }
        }

        if (invalid_area.HasIntersection(ptr->GetRect())) {
            ptr->Invalidate();
        }

        ptr->Draw();
        ptr = ptr->GetNext();
    }

    invalid_area = Rect16(); // clear invalid_area
}

void window_frame_t::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    intptr_t dif = (intptr_t)param;
    window_t *pWin = GetFocusedWindow();
    if (!pWin || !pWin->IsChildOf(this)) {
        pWin = nullptr;
    }

    switch (event) {

    case GUI_event_t::CHILD_CLICK:
        if (auto p = GetParent()) {
            p->WindowEvent(sender, event, param);
        }
        break;

    case GUI_event_t::CLICK:
        if (pWin) {
            pWin->WindowEvent(this, GUI_event_t::CLICK, nullptr);
            // pWin->SetCapture(); //item must do this - only some of them
        } else {
            // todo should not I resend event to super?
        }

        break;

    case GUI_event_t::TOUCH_CLICK:
        if (pWin) { // check if a window has focus, to not give it if it does not
            pWin = get_child_by_touch_point(event_conversion_union { .pvoid = param }.point);
        }
        if (pWin) {
            pWin->SetFocus();
            if (pWin->IsFocused()) {
                // sending click if sitting of focus failed can be harmful
                // DialogTimed resends capture events if it is not active
                // this generates unwanted click on focused window
                pWin->WindowEvent(this, GUI_event_t::CLICK, nullptr);
            }
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

            if (pNext && pNext->IsVisible()) {
                Sound_Play(eSOUND_TYPE::EncoderMove);
            } else {
                Sound_Play(eSOUND_TYPE::BlindAlert);
                break;
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
            if (pWin && !pWin->IsEnabled()) {
                pWin = pWin->GetNextEnabled();
            }
            if (pWin) {
                pWin->SetFocus();
            }
        }
        break;

    default:
        break;
    }
}

// resend event to all children
void window_frame_t::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    window_t *ptr = first_normal;
    while (ptr) {
        ptr->ScreenEvent(sender, event, param);
        ptr = ptr->GetNext();
    }
    WindowEvent(this, event, param);
}

// resend invalidation to all children
void window_frame_t::invalidate(Rect16 invalidation_rect) {
    window_t *ptr = first_normal;
    while (ptr) {
        ptr->Invalidate(invalidation_rect);
        ptr = ptr->GetNext();
    }
    if (invalidation_rect.IsEmpty()) {
        flags.invalid = true;
    } else {
        addInvalidationRect(invalidation_rect);
    }
}

// resend validate to all children
void window_frame_t::validate(Rect16 validation_rect) {
    window_t *ptr = first_normal;
    while (ptr) {
        ptr->Validate(validation_rect);
        ptr = ptr->GetNext();
    }
    invalid_area = Rect16();
}

bool window_frame_t::IsChildFocused() {
    return GetFocusedWindow() != nullptr ? (GetFocusedWindow()->GetParent() == this) : false;
}

window_t *window_frame_t::GetNextSubWin(window_t *win) const {
    if (!win) {
        return nullptr;
    }
    if (win->GetParent() != this) {
        return nullptr;
    }

    return win->GetNext();
}

window_t *window_frame_t::GetPrevSubWin(window_t *win) const {
    if (!win) {
        return nullptr;
    }
    if (win->GetParent() != this) {
        return nullptr;
    }
    window_t *tmpWin = first_normal;
    while (tmpWin && GetNextSubWin(tmpWin) != win) {
        tmpWin = GetNextSubWin(tmpWin);
    }
    return tmpWin;
}

window_t *window_frame_t::GetNextEnabledSubWin(window_t *win) const {
    if (!win) {
        return nullptr;
    }
    if (win->GetParent() != this) {
        return nullptr;
    }
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
    if (!first_normal) {
        return nullptr;
    }
    if (first_normal->IsEnabled()) {
        return first_normal;
    }
    return GetNextEnabledSubWin(first_normal);
}

window_t *window_frame_t::GetNextSubWin(window_t *win, Rect16 intersection_rect) const {
    if (!win) {
        return nullptr;
    }
    if (win->GetParent() != this) {
        return nullptr;
    }

    // endless loop is safe here, last_normal window points to nullptr
    while (true) {
        win = win->GetNext();
        if (!win || win->GetRect().HasIntersection(intersection_rect)) {
            return win;
        }
    }
}

window_t *window_frame_t::GetPrevSubWin(window_t *win, Rect16 intersection_rect) const {
    if (!win) {
        return nullptr;
    }
    if (win->GetParent() != this) {
        return nullptr;
    }
    window_t *tmpWin = first_normal;
    while (tmpWin && GetNextSubWin(tmpWin, intersection_rect) != win) {
        tmpWin = GetNextSubWin(tmpWin, intersection_rect);
    }
    return tmpWin;
}

window_t *window_frame_t::GetNextEnabledSubWin(window_t *win, Rect16 intersection_rect) const {
    if (!win) {
        return nullptr;
    }
    if (win->GetParent() != this) {
        return nullptr;
    }

    // endless loop is safe here, last_normal window points to nullptr
    while (true) {
        win = win->GetNextEnabled();
        if (!win || win->GetRect().HasIntersection(intersection_rect)) {
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
    if (!first_normal) {
        return nullptr;
    }
    if (first_normal->IsEnabled() && first_normal->GetRect().HasIntersection(intersection_rect)) {
        return first_normal;
    }
    return GetNextEnabledSubWin(first_normal, intersection_rect);
}

window_t *window_frame_t::get_child_by_touch_point(point_ui16_t point) {
    for (window_t *r = GetFirstEnabledSubWin(); r; r = GetNextEnabledSubWin(r)) {
        if (r->get_rect_for_touch().Contain(point)) {
            return r;
        }
    }
    return nullptr;
}

Rect16 window_frame_t::GenerateRect(ShiftDir_t direction, size_ui16_t sz, uint16_t distance) {
    if (!last_normal) {
        return Rect16();
    }
    return Rect16(last_normal->GetRect(), direction, sz, distance);
}

Rect16 window_frame_t::GenerateRect(Rect16::Width_t width, uint16_t distance) {
    if (!last_normal) {
        return Rect16();
    }
    return Rect16(last_normal->GetRect(), width, distance);
}

Rect16 window_frame_t::GenerateRect(Rect16::Height_t height, uint16_t distance) {
    if (!last_normal) {
        return Rect16();
    }
    return Rect16(last_normal->GetRect(), height, distance);
}

void window_frame_t::Shift(ShiftDir_t direction, uint16_t distance) {
    window_t *pWin = first_normal;
    while (pWin) {
        pWin->Shift(direction, distance);
        pWin = GetNextSubWin(pWin);
    }

    window_t::Shift(direction, distance);
}

void window_frame_t::ChildVisibilityChanged(window_t &child) {
    addInvalidationRect(child.GetRect());
}

window_t *window_frame_t::getCapturedNormalWin() const {
    return captured_normal_window;
}

bool window_frame_t::IsChildCaptured() const {
    return captured_normal_window;
}

bool window_frame_t::CaptureNormalWindow(window_t &win) {
    if (win.GetParent() != this || win.GetType() != win_type_t::normal) {
        return false;
    }
    window_t *last_captured = getCapturedNormalWin();
    if (last_captured) {
        last_captured->WindowEvent(this, GUI_event_t::CAPT_0, 0); // will not resend event to anyone
    }
    captured_normal_window = &win;
    win.WindowEvent(this, GUI_event_t::CAPT_1, 0); // will not resend event to anyone
    gui_invalidate();

    return true;
}

void window_frame_t::ReleaseCaptureOfNormalWindow() {
    if (captured_normal_window) {
        captured_normal_window->WindowEvent(this, GUI_event_t::CAPT_0, 0); // will not resend event to anyone
    }
    captured_normal_window = nullptr;
    gui_invalidate();
}

window_t *window_frame_t::GetCapturedWindow() {
    window_t *ret = window_t::GetCapturedWindow(); // this, if it can be captured or nullptr

    // rewrite ret value with valid captured subwin
    // cannot use IsCapturable or IsVisible, because it use hidden_behind_dialog flag
    // but it might be popup. At this point we are sure no dialog has capture, so we check only visible flag
    if (getCapturedNormalWin() && getCapturedNormalWin()->HasVisibleFlag()) {
        ret = getCapturedNormalWin()->GetCapturedWindow();
    }

    return ret;
}

void window_frame_t::RecursiveCall(mem_fnc fnc) {
    window_t *pWin = first_normal;
    if (!last_normal) {
        return;
    }
    while (pWin && pWin != GetNextSubWin(last_normal)) {
        std::invoke(fnc, *pWin);
        pWin = GetNextSubWin(pWin);
    }
}

void window_frame_t::set_layout(ColorLayout lt) {
    window_t::set_layout(lt);
    switch (lt) {

    case ColorLayout::red:
        RecursiveCall(&window_t::SetRedLayout);
        break;

    case ColorLayout::black:
        RecursiveCall(&window_t::SetBlackLayout);
        break;

    case ColorLayout::blue:
        RecursiveCall(&window_t::SetBlueLayout);
        break;
    }
}
