//window.cpp

#include "window.hpp"
#include "gui.hpp"
#include <algorithm> // std::find

#define WINDOW_MAX_WINDOWS 64

#define WINDOW_MAX_USERCLS 10

extern osThreadId displayTaskHandle;

window_t *window_0 = 0; //current screen window

window_t *window_popup_ptr = 0; //current popup window

//window_t *windows[WINDOW_MAX_WINDOWS];
uint16_t window_count = 0;

window_t *window_focused_ptr = 0; //current focused window

window_t *window_capture_ptr = 0; //current capture window

uint16_t window_user_class_count = 0;

int16_t window_new_id(window_t *window) {
    /*    int16_t id = -1;
    if ((window != 0) && (window_count < WINDOW_MAX_WINDOWS)) {
        id = 0;
        if (window_count == 0) //reset all pointers when starting
        {
            memset(windows, 0, WINDOW_MAX_WINDOWS * sizeof(window_t *));
            window_0 = window;
        } else //find free id
            while ((id < WINDOW_MAX_WINDOWS) && (windows[id]))
                id++;
        if (id < WINDOW_MAX_WINDOWS) { //id is valid
            windows[id] = window;      //set window pointer
            window_count++;            //increment count
        } else
            id = -1;
    }
    return id;*/
}

window_t *window_free_id(int16_t id) {
    /*   window_t *window;
    if ((window = window_ptr(id)) != 0) { //valid id and not null window pointer
        windows[id] = 0;                  //reset pointer
        window_count--;                   //decrement count
    }
    return window;*/
}

window_t *window_ptr(int16_t id) {
    //return ((id >= 0) && (id < WINDOW_MAX_WINDOWS)) ? windows[id] : 0;
}

int16_t window_create_ptr(int16_t cls_id, int16_t id_parent, rect_ui16_t rect, void *ptr) {
    /*    window_class_t *cls = class_ptr(cls_id);
    if (cls) {
        uint32_t flg = WINDOW_FLG_VISIBLE | WINDOW_FLG_INVALID;
        window_t *win = (window_t *)ptr;
        if (win == 0) {
            win = (window_t *)gui_malloc(cls->size);
            flg |= WINDOW_FLG_FREEMEM;
        }
        if (win == 0)
            return -1;
        int16_t id = window_new_id(win);
        if (id >= 0) {
            win->id = id;
            win->id_parent = id_parent;
            //win->cls = cls;
            win->flg = flg;
            win->rect = rect;
            //win->event = cls->event;
            win->f_tag = 0;
            if (cls->init)
                cls->init(win);
            return id;
        }
        if (win && (ptr == 0))
            gui_free(win);
    }
    return -1;*/
}

void window_destroy(int16_t id) {
    window_t *window = window_free_id(id);
    uint16_t count = window_count;
    if (window != 0) {
        if (window->HasTimer())
            gui_timers_delete_by_window_id(window->id);
        window->id = -1;
        //if (window->f_parent)
        //    window_destroy_children(id);
        //if (window->cls->done)
        //    window->cls->done(window);
        //if (window->f_freemem)
        //    gui_free(window);
        if (window == window_capture_ptr)
            window_capture_ptr = 0;
        if (window == window_focused_ptr)
            window_focused_ptr = 0;
        if (window == window_popup_ptr)
            window_popup_ptr = 0;
        //if (window == window_0) window_0 = 0;
        if (count == 0)
            window_0 = 0;
    }
}

int16_t window_focused(void) {
    return window_focused_ptr ? window_focused_ptr->id : 0;
}

int16_t window_capture(void) {
    return window_capture_ptr ? window_capture_ptr->id : 0;
}

bool window_t::IsVisible() const { return f_visible == true; }
bool window_t::IsEnabled() const { return f_enabled == true; }
bool window_t::IsInvalid() const { return f_invalid == true; }
bool window_t::IsFocused() const { return f_focused == true; }
bool window_t::IsCapture() const { return f_capture == true; }
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

    if (window_focused_ptr) {
        window_focused_ptr->f_focused = 0;
        window_focused_ptr->Invalidate();
        window_focused_ptr->event(window_focused_ptr, WINDOW_EVENT_FOCUS0, 0); //will not resend event to anyone
    }
    window_focused_ptr = this;
    f_focused = 1;
    Invalidate();
    event(this, WINDOW_EVENT_FOCUS1, 0); //will not resend event to anyone
    gui_invalidate();
}

void window_t::SetCapture() {

    if (f_visible && f_enabled) {
        if (window_capture_ptr) {
            window_capture_ptr->f_capture = 0;
            window_capture_ptr->event(window_capture_ptr, WINDOW_EVENT_CAPT_0, 0); //will not resend event to anyone
        }
        window_capture_ptr = this;
        f_capture = 1;
        event(this, WINDOW_EVENT_CAPT_1, 0); //will not resend event to anyone
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

window_t::window_t(window_t *parent, window_t *prev, rect_ui16_t rect)
    : parent(parent)
    , next(nullptr)
    , rect(rect) {
    Enable();
    Show();
    Invalidate();
    if (prev)
        prev->SetNext(this);
    if (rect.w && rect.h)
        display::FillRect(rect, color_back);
}

window_t::~window_t() {
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

void window_t::unconditionalDraw() {
    display::FillRect(rect, color_back);
}

void window_t::Event(window_t *sender, uint8_t ev, void *param) {
    if (event(sender, ev, param) == 0) {
        //if event was not handled send it to parent
        if (parent) {
            parent->Event(sender, ev, param);
        }
    }
}

void window_t::DispatchEvent(window_t *sender, uint8_t ev, void *param) {
    dispatchEvent(sender, ev, param);
}

//frame does something else - resends to all childern
void window_t::dispatchEvent(window_t *sender, uint8_t ev, void *param) {
    event(sender, ev, param);
}
