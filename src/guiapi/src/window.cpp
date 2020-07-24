//window.cpp

#include "window.hpp"
#include "gui.hpp"
#include <algorithm> // std::find
#include "ScreenHandler.hpp"

extern osThreadId displayTaskHandle;

bool window_t::IsVisible() const { return f_visible == true; }
bool window_t::IsEnabled() const { return f_enabled == true; }
bool window_t::IsInvalid() const { return f_invalid == true; }
bool window_t::IsFocused() const { return GetFocusedWindow() == this; }
bool window_t::IsCapture() const { return GetCapturedWindow() == this; }
bool window_t::HasTimer() const { return f_timer == true; }
bool window_t::IsDialog() const { return f_dialog == true; }
void window_t::Validate(rect_ui16_t validation_rect) {
    //todo check validation_rect intersection
    f_invalid = false;
    invalidate(validation_rect);
    gui_invalidate();
}

void window_t::Invalidate(rect_ui16_t validation_rect) {
    //todo check validation_rect intersection
    f_invalid = true;
    invalidate(validation_rect);
    gui_invalidate();
}

//frame will invalidate childern
void window_t::invalidate(rect_ui16_t validation_rect) {
}

//frame will validate childern
void window_t::validate(rect_ui16_t validation_rect) {
}

void window_t::SetHasTimer() { f_timer = true; }
void window_t::ClrHasTimer() { f_timer = false; }
void window_t::Enable() { f_enabled = true; }
void window_t::Disable() { f_enabled = false; }

void window_t::SetFocus() {
    if (!f_visible || !f_enabled)
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

    if (f_visible && f_enabled) {
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
        f_visible = 1;
        Invalidate();
    }
}

void window_t::Hide() {
    if (IsVisible()) {
        f_visible = 0;
        Invalidate();
    }
}

color_t window_t::GetBackColor() const { return color_back; }

void window_t::SetBackColor(color_t clr) {
    color_back = clr;
    Invalidate();
}

window_t::window_t(window_t *parent, rect_ui16_t rect, bool dialog)
    : parent(parent)
    , next(nullptr)
    , flg(0)
    , rect(rect)
    , color_back(gui_defaults.color_back) {
    f_dialog = dialog;
    Disable();
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
    if (next) {
        return (next->IsEnabled()) ? next : next->GetNextEnabled();
    } else {
        return nullptr;
    }
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
    if (IsInvalid() && IsVisible() && rect.w && rect.h) {
        unconditionalDraw();
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

//frame does something else - resends to all childern
void window_t::screenEvent(window_t *sender, uint8_t ev, void *param) {
    windowEvent(sender, ev, param);
}
void window_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK && parent)
        parent->windowEvent(this, event, param);
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
