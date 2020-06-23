//window.c

#include "window.h"
#include "window_menu.h"
#include "gui.h"

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

int16_t window_id(window_t *ptr) {
    return (ptr) ? ptr->id : -1;
}

int16_t window_register_class(window_class_t *cls) {
    if ((cls) && (window_user_class_count < WINDOW_MAX_USERCLS)) {
        window_user_classes[window_user_class_count] = cls;
        return WINDOW_CLS_USER + window_user_class_count++;
    }
    return -1;
}

int16_t window_create(int16_t cls_id, int16_t id_parent, rect_ui16_t rect) {
    return window_create_ptr(cls_id, id_parent, rect, 0);
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
            win->event = cls->event;
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

int16_t window_parent(int16_t id) {
    window_t *win;
    if ((id >= 0) && (id < WINDOW_MAX_WINDOWS) && ((win = windows[id]) != 0))
        return win->id_parent;
    return -1;
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
        if (window_is_enabled(id))
            return id;
    return -1;
}

int16_t window_next_enabled(int16_t id) {
    while ((id = window_next(id)) >= 0)
        if (window_is_enabled(id))
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
        if (window_is_enabled(id))
            count++;
        while ((id = window_next(id)) >= 0)
            if (window_is_enabled(id))
                count++;
    }
    return count;
}

int window_is_visible(int16_t id) {
    window_t *window;
    return ((window = window_ptr(id)) != 0) ? window->f_visible : 0;
}

int window_is_enabled(int16_t id) {
    window_t *window;
    return ((window = window_ptr(id)) != 0) ? window->f_enabled : 0;
}

int window_is_invalid(int16_t id) {
    window_t *window;
    return ((window = window_ptr(id)) != 0) ? window->f_invalid : 0;
}

int window_is_focused(int16_t id) {
    window_t *window;
    return ((window = window_ptr(id)) != 0) ? window->f_focused : 0;
}

int window_is_capture(int16_t id) {
    window_t *window;
    return ((window = window_ptr(id)) != 0) ? window->f_capture : 0;
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

void window_validate(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        window->f_invalid = 0;
}

void window_invalidate(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        window->f_invalid = 1;
        gui_invalidate();
    }
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

void window_set_tag(int16_t id, uint8_t tag) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        window->f_tag = tag;
}

uint8_t window_get_tag(int16_t id) {
    window_t *window;
    return ((window = window_ptr(id)) != 0) ? window->f_tag : 0;
}

void window_set_text(int16_t id, const char *text) {
    window_t *window = window_ptr(id);
    if (window == NULL)
        return;

    switch (window->cls->cls_id) {
    case WINDOW_CLS_TEXT:
        ((window_text_t *)window)->text = (char *)text;
        break;
    case WINDOW_CLS_ROLL_TEXT:
        ((window_roll_text_t *)window)->text = (char *)text;
        break;
    }
    _window_invalidate((window_t *)window);
}

char *window_get_text(int16_t id) {
    window_t *window = window_ptr(id);
    if (window == NULL)
        return 0;

    switch (window->cls->cls_id) {
    case WINDOW_CLS_TEXT:
        return ((window_text_t *)window)->text;
    case WINDOW_CLS_ROLL_TEXT:
        return ((window_roll_text_t *)window)->text;
    }
    return 0;
}

void window_set_value(int16_t id, float value) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_NUMB:
            ((window_numb_t *)window)->value = value;
            break;
        case WINDOW_CLS_SPIN:
            if (value < ((window_spin_t *)window)->min)
                value = ((window_spin_t *)window)->min;
            if (value > ((window_spin_t *)window)->max)
                value = ((window_spin_t *)window)->max;
            ((window_spin_t *)window)->window.value = value;
            ((window_spin_t *)window)->index = (int)((((window_spin_t *)window)->window.value - ((window_spin_t *)window)->min) / ((window_spin_t *)window)->step);
            break;
        case WINDOW_CLS_PROGRESS:
            if (value < ((window_progress_t *)window)->min)
                value = ((window_progress_t *)window)->min;
            if (value > ((window_progress_t *)window)->max)
                value = ((window_progress_t *)window)->max;
            ((window_progress_t *)window)->value = value;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

float window_get_value(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_NUMB:
            return ((window_numb_t *)window)->value;
        case WINDOW_CLS_SPIN:
            return ((window_spin_t *)window)->window.value;
        }
    }
    return 0;
}

void window_set_format(int16_t id, const char *format) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_NUMB:
            ((window_numb_t *)window)->format = (char *)format;
            break;
        case WINDOW_CLS_SPIN:
            ((window_spin_t *)window)->window.format = (char *)format;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

const char *window_get_format(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_NUMB:
            return ((window_numb_t *)window)->format;
        case WINDOW_CLS_SPIN:
            return ((window_spin_t *)window)->window.format;
        }
    }
    return 0;
}

void window_set_color_back(int16_t id, color_t clr) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_FRAME:
            ((window_frame_t *)window)->color_back = clr;
            break;
        case WINDOW_CLS_TEXT:
            ((window_text_t *)window)->color_back = clr;
            break;
        case WINDOW_CLS_ROLL_TEXT:
            ((window_roll_text_t *)window)->color_back = clr;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

color_t window_get_color_back(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_TEXT:
            return ((window_text_t *)window)->color_back;
        case WINDOW_CLS_ROLL_TEXT:
            return ((window_roll_text_t *)window)->color_back;
        }
    }
    return COLOR_BLACK;
}

void window_set_color_text(int16_t id, color_t clr) {
    window_t *window = window_ptr(id);
    if (window == NULL)
        return;

    switch (window->cls->cls_id) {
    case WINDOW_CLS_TEXT:
        ((window_text_t *)window)->color_text = clr;
        break;
    case WINDOW_CLS_ROLL_TEXT:
        ((window_roll_text_t *)window)->color_text = clr;
        break;
    }
    _window_invalidate((window_t *)window);
}

color_t window_get_color_text(int16_t id) {
    window_t *window = window_ptr(id);
    if (window == NULL)
        return COLOR_BLACK;

    switch (window->cls->cls_id) {
    case WINDOW_CLS_TEXT:
        return ((window_text_t *)window)->color_text;
    case WINDOW_CLS_ROLL_TEXT:
        return ((window_roll_text_t *)window)->color_text;
    }
    return COLOR_BLACK;
}

void window_set_focus(int16_t id) {
    window_t *window = window_ptr(id);
    if (window == 0)
        return;
    if (!window->f_visible || !window->f_enabled)
        return;

    if (window_focused_ptr) {
        window_focused_ptr->f_focused = 0;
        window_focused_ptr->f_invalid = 1;
        if (window_focused_ptr->event)
            window_focused_ptr->event(window_focused_ptr, WINDOW_EVENT_FOCUS0, 0);
    }
    window_focused_ptr = window;
    window->f_focused = 1;
    window->f_invalid = 1;
    if (window->event)
        window->event(window, WINDOW_EVENT_FOCUS1, 0);
    gui_invalidate();
}

void window_set_capture(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        if (window->f_visible && window->f_enabled && window->event) {
            if (window_capture_ptr) {
                window_capture_ptr->f_capture = 0;
                if (window_capture_ptr->event)
                    window_capture_ptr->event(window_capture_ptr, WINDOW_EVENT_CAPT_0, 0);
            }
            window_capture_ptr = window;
            window->f_capture = 1;
            window->event(window, WINDOW_EVENT_CAPT_1, 0);
            gui_invalidate();
        }
    }
}

void window_enable(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        window->f_enabled = 1;
}

void window_disable(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        window->f_enabled = 0;
}

void window_show(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        if ((window->f_visible) == 0) {
            window->f_visible = 1;
            _window_invalidate((window_t *)window);
        }
    }
}

void window_hide(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        if (window->f_visible) {
            window->f_visible = 0;
            _window_invalidate((window_t *)window);
        }
    }
}

void window_set_padding(int16_t id, padding_ui8_t padding) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_TEXT:
            ((window_text_t *)window)->padding = padding;
            break;
        case WINDOW_CLS_ROLL_TEXT:
            ((window_roll_text_t *)window)->padding = padding;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

void window_set_alignment(int16_t id, uint8_t alignment) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_TEXT:
            ((window_text_t *)window)->alignment = alignment;
            break;
        case WINDOW_CLS_ROLL_TEXT:
            ((window_roll_text_t *)window)->alignment = alignment;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

void window_set_item_count(int16_t id, int count) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_LIST:
            ((window_list_t *)window)->count = count;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

int window_get_item_count(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_LIST:
            return ((window_list_t *)window)->count;
        case WINDOW_CLS_SPIN:
            return ((window_spin_t *)window)->count;
        }
    }
    return -1;
}

void window_set_item_index(int16_t id, int index) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_MENU:
            window_menu_set_item_index(window, index);
            break;
        case WINDOW_CLS_LIST:
            if (((window_list_t *)window)->count > index) {
                ((window_list_t *)window)->index = index;
            }
            break;
        case WINDOW_CLS_SPIN:
            if (((window_spin_t *)window)->count > index) {
                ((window_spin_t *)window)->index = index;
                ((window_spin_t *)window)->window.value = ((window_spin_t *)window)->min + ((window_spin_t *)window)->step * ((window_spin_t *)window)->index;
            }
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

int window_get_item_index(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_LIST:
            return ((window_list_t *)window)->index;
        case WINDOW_CLS_SPIN:
            return ((window_spin_t *)window)->index;
        }
    }
    return -1;
}

void window_set_top_index(int16_t id, int top_index) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_LIST:
            ((window_list_t *)window)->top_index = top_index;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

int window_get_top_index(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_LIST:
            return ((window_list_t *)window)->top_index;
        }
    }
    return -1;
}

void window_set_icon_id(int16_t id, uint16_t id_res) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_ICON:
            ((window_icon_t *)window)->id_res = id_res;
            break;
        }
        _window_invalidate((window_t *)window);
    }
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

void window_set_min(int16_t id, float min) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            ((window_spin_t *)window)->min = min;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

float window_get_min(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            return ((window_spin_t *)window)->min;
        }
    return 0;
}

void window_set_max(int16_t id, float max) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            ((window_spin_t *)window)->max = max;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

float window_get_max(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            return ((window_spin_t *)window)->max;
        }
    return 0;
}

void window_set_step(int16_t id, float step) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            ((window_spin_t *)window)->step = step;
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

float window_get_step(int16_t id) {
    window_t *window;
    if ((window = window_ptr(id)) != 0)
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            return ((window_spin_t *)window)->step;
        }
    return 0;
}

void window_set_min_max(int16_t id, float min, float max) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            if (((window_spin_t *)window)->window.value < min)
                ((window_spin_t *)window)->window.value = min;
            if (((window_spin_t *)window)->window.value > max)
                ((window_spin_t *)window)->window.value = max;
            ((window_spin_t *)window)->min = min;
            ((window_spin_t *)window)->max = max;
            ((window_spin_t *)window)->count = (int)((max - min) / ((window_spin_t *)window)->step + 1.5F);
            ((window_spin_t *)window)->index = (int)((((window_spin_t *)window)->window.value - min) / ((window_spin_t *)window)->step);
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

void window_set_min_max_step(int16_t id, float min, float max, float step) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_SPIN:
            if (((window_spin_t *)window)->window.value < min)
                ((window_spin_t *)window)->window.value = min;
            if (((window_spin_t *)window)->window.value > max)
                ((window_spin_t *)window)->window.value = max;
            ((window_spin_t *)window)->min = min;
            ((window_spin_t *)window)->max = max;
            ((window_spin_t *)window)->step = step;
            ((window_spin_t *)window)->count = (int)((max - min) / step + 1.5F);
            ((window_spin_t *)window)->index = (int)((((window_spin_t *)window)->window.value - min) / step);
            break;
        }
        _window_invalidate((window_t *)window);
    }
}

void window_set_item_callback(int16_t id, window_list_item_t *fnc) {
    window_t *window;
    if ((window = window_ptr(id)) != 0) {
        switch (window->cls->cls_id) {
        case WINDOW_CLS_LIST:
            ((window_list_t *)window)->list_item = fnc;
        }
    }
}

void window_dispatch_event(window_t *window, uint8_t event, void *param) {
    if (window && window->event)
        window->event(window, event, param);
}
