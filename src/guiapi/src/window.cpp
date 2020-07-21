//window.cpp

#include "window.hpp"
#include "gui.hpp"
#include <algorithm> // std::find
#include "ScreenHandler.hpp"

extern osThreadId displayTaskHandle;

window_t *window_popup_ptr = 0; //current popup window

/*
void window_destroy(int16_t id) {
    window_t *window = window_free_id(id);
    uint16_t count = window_count;
    if (window != 0) {
        if (window->HasTimer())
            gui_timers_delete_by_window_id(window->id);
        window->id = -1;
        if (window->f_parent)
            window_destroy_children(id);
        if (window->cls->done)
            window->cls->done(window);
        if (window->f_freemem)
            gui_free(window);
        if (window == window_capture_ptr)
            window_capture_ptr = 0;
        if (window == window_focused_ptr)
            window_focused_ptr = 0;
        if (window == window_popup_ptr)
            window_popup_ptr = 0;
        if (window == window_0) window_0 = 0;
        if (count == 0)
            window_0 = 0;
    }
}
*/
bool window_t::IsVisible() const { return f_visible == true; }
bool window_t::IsEnabled() const { return f_enabled == true; }
bool window_t::IsInvalid() const { return f_invalid == true; }
bool window_t::IsFocused() const { return GetFocusedWindow() == this; }
bool window_t::IsCapture() const { return GetCapturedWindow() == this; }
bool window_t::HasTimer() const { return f_timer == true; }
void window_t::Validate() { f_invalid = false; }

void window_t::Invalidate() {
    f_invalid = true;
    gui_invalidate();
}

void window_t::SetTag(uint8_t tag) { f_tag = tag; };
uint8_t window_t::GetTag() const { return f_tag; }

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

window_t::window_t(window_t *parent, rect_ui16_t rect)
    : parent(parent)
    , next(nullptr)
    , f_tag(0)
    , flg(0)
    , rect(rect)
    , color_back(gui_defaults.color_back) {
    Disable();
    Show();
    Invalidate();
    if (parent)
        parent->push_back(this);
}

window_t::window_t(rect_ui16_t rect)
    : window_t(Screens::Access()->Get(), rect) {
}

window_t::~window_t() {
    if (GetFocusedWindow() == this)
        focused_ptr = nullptr;
    if (GetCapturedWindow() == this)
        capture_ptr = nullptr;

    if (GetParent())
        GetParent()->Unregister();
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
void window_t::push_back(window_t *win) {
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
