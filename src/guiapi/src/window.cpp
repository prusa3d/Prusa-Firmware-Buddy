//window.c

#include "window.hpp"
#include "window_menu.h"
#include "gui.hpp"
#include <algorithm> // std::find

#define WINDOW_MAX_WINDOWS 64

#define WINDOW_MAX_USERCLS 10

extern osThreadId displayTaskHandle;

window_t *window_0 = 0; //current screen window

window_t *window_popup_ptr = 0; //current popup window

window_t *windows[WINDOW_MAX_WINDOWS];
uint16_t window_count = 0;

window_t *window_focused_ptr = 0; //current focused window

window_t *window_capture_ptr = 0; //current capture window

// warning: initializing non-local variable with non-const expression depending on uninitialized non-local variable 'window_class_frame'
// [cppcoreguidelines-interfaces-global-init]
const window_class_t *window_classes[] = {
    (window_class_t *)(&window_class_frame),     //  0  FRAME
    (window_class_t *)(&window_class_text),      //  1  TEXT
    (window_class_t *)(&window_class_numb),      //  2  NUMB
    (window_class_t *)(&window_class_icon),      //  3  ICON
    (window_class_t *)(&window_class_list),      //  4  LIST
    0,                                           //  5  EDIT
    (window_class_t *)(&window_class_spin),      //  6  SPIN
    0,                                           //  7  TXIC
    (window_class_t *)(&window_class_term),      //  8  TERM
    (window_class_t *)(&window_class_menu),      //  9  MENU
    (window_class_t *)(&window_class_msgbox),    //  10  MSGBOX
    (window_class_t *)(&window_class_progress),  //  11  PROGRESS
    (window_class_t *)(&window_class_qr),        //  12  QR
    (window_class_t *)(&window_class_roll_text), // 13 ROLL_TEXT
};

const uint16_t window_class_count = sizeof(window_classes) / sizeof(window_class_t *);

window_class_t *window_user_classes[WINDOW_MAX_USERCLS];

uint16_t window_user_class_count = 0;

int16_t window_new_id(window_t *window) {
    int16_t id = -1;
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
    return id;
}

window_t *window_free_id(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) { //valid id and not null window pointer
        windows[id] = 0;                  //reset pointer
        window_count--;                   //decrement count
    }
    return window;
}

window_class_t *class_ptr(int16_t cls_id) {
    window_class_t *cls = 0;
    if ((cls_id >= 0) && (cls_id < window_class_count) && ((cls = (window_class_t *)window_classes[cls_id]) != 0))
        return cls;
    if ((cls_id >= WINDOW_CLS_USER) && (cls_id < (WINDOW_CLS_USER + window_user_class_count)) && ((cls = window_user_classes[cls_id - WINDOW_CLS_USER]) != 0))
        return cls;
    return 0;
}

window_t *window_ptr(int16_t id) {
    return ((id >= 0) && (id < WINDOW_MAX_WINDOWS)) ? windows[id] : 0;
}

int16_t window_register_class(window_class_t *cls) {
    if ((cls) && (window_user_class_count < WINDOW_MAX_USERCLS)) {
        window_user_classes[window_user_class_count] = cls;
        return WINDOW_CLS_USER + window_user_class_count++;
    }
    return -1;
}

int16_t window_create_ptr(int16_t cls_id, int16_t id_parent, rect_ui16_t rect, void *ptr) {
    window_class_t *cls = class_ptr(cls_id);
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
            win->cls = cls;
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
    return -1;
}

void window_destroy(int16_t id) {
    window_t *window = window_free_id(id);
    uint16_t count = window_count;
    if (window != 0) {
        if (window->f_timer)
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
        //if (window == window_0) window_0 = 0;
        if (count == 0)
            window_0 = 0;
    }
}

void window_destroy_children(int16_t id) {
    window_t *window;
    int16_t id_child;
    for (id_child = 0; id_child < WINDOW_MAX_WINDOWS; id_child++)
        if (((window = windows[id_child]) != 0) && (window->id_parent == id))
            window_destroy(id_child);
}

int16_t window_focused(void) {
    return window_focused_ptr ? window_focused_ptr->id : 0;
}

int16_t window_capture(void) {
    return window_capture_ptr ? window_capture_ptr->id : 0;
}

int16_t window_prev(int16_t id) {
    window_t *win;
    if ((id >= 0) && (id < WINDOW_MAX_WINDOWS) && ((win = windows[id]) != 0)) {
        int16_t id_parent = win->id_parent;
        while (--id >= 0)
            if ((win = windows[id]) != 0)
                if (win->id_parent == id_parent)
                    return id;
    }
    return -1;
}

int16_t window_next(int16_t id) {
    window_t *win;
    if ((id >= 0) && (id < WINDOW_MAX_WINDOWS) && ((win = windows[id]) != 0)) {
        int16_t id_parent = win->id_parent;
        while (++id < WINDOW_MAX_WINDOWS)
            if ((win = windows[id]) != 0)
                if (win->id_parent == id_parent)
                    return id;
    }
    return -1;
}

int16_t window_prev_enabled(int16_t id) {
    while ((id = window_prev(id)) >= 0)
        if (window_ptr(id) ? window_ptr(id)->IsEnabled() : false)
            return id;
    return -1;
}

int16_t window_next_enabled(int16_t id) {
    while ((id = window_next(id)) >= 0)
        if (window_ptr(id) != 0 ? window_ptr(id)->IsEnabled() : 0)
            return id;
    return -1;
}

int16_t window_first_child(int16_t id) {
    int16_t id_parent = id;
    window_t *win;
    if ((id >= 0) && (id < WINDOW_MAX_WINDOWS) && ((win = windows[id]) != 0)) {
        while (++id < WINDOW_MAX_WINDOWS)
            if ((win = windows[id]) != 0)
                if (win->id_parent == id_parent)
                    return id;
    }
    return -1;
}

int window_child_count(int16_t id) {
    int count = 0;
    if ((id = window_first_child(id)) >= 0) {
        count++;
        while ((id = window_next(id)) >= 0)
            count++;
    }
    return count;
}

int window_enabled_child_count(int16_t id) {
    int count = 0;
    if ((id = window_first_child(id)) >= 0) {
        if (window_ptr(id) != 0 ? window_ptr(id)->IsEnabled() : 0)
            count++;
        while ((id = window_next(id)) >= 0)
            if (window_ptr(id) != 0 ? window_ptr(id)->IsEnabled() : 0)
                count++;
    }
    return count;
}

void window_draw(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        if (window->cls->draw)
            window->cls->draw(window);
}

void window_draw_children(int16_t id) {
    window_t *window;
    int16_t id_child;
    for (id_child = 0; id_child < WINDOW_MAX_WINDOWS; id_child++)
        if (((window = windows[id_child]) != 0) && (window->id_parent == id)) {
            if (window_popup_ptr && window_popup_ptr->id != window->id_parent) {
                if (rect_empty_ui16(rect_intersect_ui16(window_popup_ptr->rect, window->rect)))
                    if (window->cls->draw)
                        window->cls->draw(window);
            } else if (window->cls->draw)
                window->cls->draw(window);
        }
}

void window_t::Invalidate() {
    f_invalid = 1;
    gui_invalidate();
}

void window_validate_children(int16_t id) {
    window_t *window;
    int16_t id_child;
    for (id_child = 0; id_child < WINDOW_MAX_WINDOWS; id_child++)
        if (((window = windows[id_child]) != 0) && (window->id_parent == id))
            window->f_invalid = 0;
}

void window_invalidate_children(int16_t id) {
    window_t *window;
    int16_t id_child;
    for (id_child = 0; id_child < WINDOW_MAX_WINDOWS; id_child++)
        if (((window = windows[id_child]) != 0) && (window->id_parent == id))
            window->f_invalid = 1;
    gui_invalidate();
}

void window_t::SetFocus() {
    if (!f_visible || !f_enabled)
        return;

    if (window_focused_ptr) {
        window_focused_ptr->f_focused = 0;
        window_focused_ptr->f_invalid = 1;
        window_focused_ptr->event(window_focused_ptr, WINDOW_EVENT_FOCUS0, 0); //will not resend event to anyone
    }
    window_focused_ptr = this;
    f_focused = 1;
    f_invalid = 1;
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
    if (!f_visible) {
        f_visible = 1;
        Invalidate();
    }
}

void window_t::Hide() {
    if (f_visible) {
        f_visible = 0;
        Invalidate();
    }
}

void window_t::SetBackColor(color_t clr) {
    color_back = clr;
    Invalidate();
}

uint16_t window_get_icon_id(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_ICON:
            return ((window_icon_t *)window)->id_res;
        }
    }
    return 0;
}

/*
window_t::window_t(int16_t cls_id, int16_t id_parent, rect_ui16_t rect)
    : window_t() {
    window_class_t *cls = class_ptr(cls_id);
    if (cls) {
        uint32_t flg = WINDOW_FLG_VISIBLE | WINDOW_FLG_INVALID;

        int16_t id = window_new_id(this);
        if (id >= 0) {
            this->id = id;
            this->id_parent = id_parent;
            this->cls = cls;
            this->flg = flg;
            this->rect = rect;
            this->event = cls->event;
            this->f_tag = 0;
            if (cls->init)
                cls->init(this);
        }
    }
}*/

window_t::window_t(window_t *parent, window_t *prev, rect_ui16_t rect)
    : parent(parent)
    , next(nullptr)
    , flg(WINDOW_FLG_ENABLED | WINDOW_FLG_VISIBLE | WINDOW_FLG_INVALID)
    , rect(rect) {
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
    if (IsInvalid() && IsVisible()) {
        draw();
        Validate();
    }
}

void window_t::draw() {
#if _DEBUG
    display::FillRect(rect, COLOR_BLUE);
#else
    display::FillRect(rect, color_back);
#endif
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
