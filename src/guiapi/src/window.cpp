//window.cpp

#include "window.hpp"
#include "gui.hpp"
#include <algorithm> // std::find
#include "ScreenHandler.hpp"

extern osThreadId displayTaskHandle;

bool window_t::IsVisible() const { return flag_visible; }
bool window_t::IsEnabled() const { return flag_enabled; }
bool window_t::IsInvalid() const { return flag_invalid; }
bool window_t::IsFocused() const { return GetFocusedWindow() == this; }
bool window_t::IsCaptured() const { return GetCapturedWindow() == this; }
bool window_t::HasTimer() const { return flag_timer; }
bool window_t::IsDialog() const { return flag_dialog == is_dialog_t::yes; }
void window_t::Validate(Rect16 validation_rect) {
    //todo check validation_rect intersection
    flag_invalid = false;
    validate(validation_rect);
}

void window_t::Invalidate(Rect16 validation_rect) {
    //todo check validation_rect intersection
    flag_invalid = true;
    invalidate(validation_rect);
    gui_invalidate();
}

//frame will invalidate children
void window_t::invalidate(Rect16 validation_rect) {
}

//frame will validate children
void window_t::validate(Rect16 validation_rect) {
}

void window_t::SetHasTimer() { flag_timer = true; }
void window_t::ClrHasTimer() { flag_timer = false; }
void window_t::Enable() { flag_enabled = true; }
void window_t::Disable() { flag_enabled = false; }

void window_t::SetFocus() {
    if (!flag_visible || !flag_enabled)
        return;

    if (focused_ptr) {
        focused_ptr->Invalidate();
        focused_ptr->windowEvent(focused_ptr, WINDOW_EVENT_FOCUS0, 0); //will not resend event to anyone
    }
    focused_ptr = this;
    Invalidate();
    windowEvent(this, WINDOW_EVENT_FOCUS1, 0); //will not resend event to anyone
    gui_invalidate();
}

void window_t::SetCapture() {

    if (flag_visible && flag_enabled) {
        if (capture_ptr) {
            capture_ptr->windowEvent(capture_ptr, WINDOW_EVENT_CAPT_0, 0); //will not resend event to anyone
        }
        capture_ptr = this;
        windowEvent(this, WINDOW_EVENT_CAPT_1, 0); //will not resend event to anyone
        gui_invalidate();
    }
}

void window_t::Show() {
    if (!IsVisible()) {
        flag_visible = true;
        Invalidate();
    }
}

void window_t::Hide() {
    if (IsVisible()) {
        flag_visible = false;
        Invalidate();
    }
}

color_t window_t::GetBackColor() const { return color_back; }

void window_t::SetBackColor(color_t clr) {
    color_back = clr;
    Invalidate();
}

window_t::window_t(window_t *parent, Rect16 rect, is_dialog_t dialog, is_closed_on_click_t close)
    : parent(parent)
    , next(nullptr)
    , flg(0)
    , rect(rect)
    , color_back(GuiDefaults::ColorBack) {
    flag_dialog = dialog;
    flag_close_on_click = close;
    close == is_closed_on_click_t::yes ? Enable() : Disable();
    Show();
    Invalidate();
    if (parent)
        parent->RegisterSubWin(this);
}

window_t::~window_t() {
    if (GetFocusedWindow() == this)
        focused_ptr = nullptr;
    if (GetCapturedWindow() == this)
        capture_ptr = nullptr;

    //no need to unregister non dialogs
    if (GetParent() && IsDialog())
        GetParent()->UnregisterSubWin(this);
}

void window_t::SetNext(window_t *nxt) {
    next = nxt;
}

/*
void window_t::SetPrev(window_t *prv) {
    prev = prv;
}
*/

void window_t::SetParent(window_t *par) {
    parent = par;
}

window_t *window_t::GetNext() const {
    return next;
}

/*
window_t *window_t::GetPrev() const {
    return prev;
}
*/

window_t *window_t::GetNextEnabled() const {
    if (next)
        return (next->IsEnabled()) ? next : next->GetNextEnabled();
    return nullptr;
}

/*
window_t *window_t::GetPrevEnabled() const {
    if (prev){
        return (prev->IsEnabled()) ? prev : prev->GetPrevEnabled();
    } else {
        return nullptr;
    }
}
*/

window_t *window_t::GetParent() const {
    return parent;
}

void window_t::Draw() {
    draw();
}

void window_t::draw() {
    if (IsInvalid() && rect.Width() && rect.Height()) {
        if (IsVisible()) {
            unconditionalDraw();
        } else {
            display::FillRect(rect, color_back);
        }
        Validate();
    }
}

//window does not support subwindow elements, but window_frame does
void window_t::RegisterSubWin(window_t *win) {
}

void window_t::unconditionalDraw() {
    display::FillRect(rect, color_back);
}

void window_t::WindowEvent(window_t *sender, uint8_t ev, void *param) {
    windowEvent(sender, ev, param);
}

void window_t::ScreenEvent(window_t *sender, uint8_t ev, void *param) {
    screenEvent(sender, ev, param);
}

//frame does something else - resend to all children
void window_t::screenEvent(window_t *sender, uint8_t ev, void *param) {
    windowEvent(sender, ev, param);
}
void window_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK && parent) {
        if (flag_close_on_click == is_closed_on_click_t::yes) {
            Screens::Access()->Close();
        } else {
            parent->windowEvent(this, event, param);
        }
    }
}
/*****************************************************************************/
//static

window_t *window_t::focused_ptr = nullptr;
window_t *window_t::capture_ptr = nullptr;

window_t *window_t::GetFocusedWindow() {
    return focused_ptr;
}

window_t *window_t::GetCapturedWindow() {
    return capture_ptr;
}
