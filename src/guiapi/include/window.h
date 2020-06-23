//window.h
#ifndef _WINDOW_H
#define _WINDOW_H

#include <inttypes.h>
#include "guitypes.h"

//window class identifiers
#define WINDOW_CLS_FRAME     0   // FRAME - basic container class
#define WINDOW_CLS_TEXT      1   // TEXT - aligned singlecolor text
#define WINDOW_CLS_NUMB      2   // NUMB - aligned singlecolor formated number
#define WINDOW_CLS_ICON      3   // ICON - small image with left-top offset
#define WINDOW_CLS_LIST      4   // LIST - vertical or horizontal list (text-icon pairs)
#define WINDOW_CLS_EDIT      5   // EDIT - text editor (editable 'TEXT') - minor
#define WINDOW_CLS_SPIN      6   // SPIN - numeric editor (editable 'NUMB')
#define WINDOW_CLS_TXIC      7   // TXIC - text + icon
#define WINDOW_CLS_TERM      8   // TERM - terminal
#define WINDOW_CLS_MENU      9   // MENU - menu
#define WINDOW_CLS_MSGBOX    10  // MSGBOX - messagebox with configurable buttons and icon
#define WINDOW_CLS_PROGRESS  11  // PROGRESS - progress bar with text
#define WINDOW_CLS_QR        12  // QR - QR Code
#define WINDOW_CLS_ROLL_TEXT 13  // ROLL text - text too long for display width
#define WINDOW_CLS_USER      128 // USER - user defined window classes (WINDOW_CLS_USER+n)

//window flags
#define WINDOW_FLG_VISIBLE  0x00000001 // is visible
#define WINDOW_FLG_ENABLED  0x00000002 // is enabled (can be focused)
#define WINDOW_FLG_INVALID  0x00000004 // content is invalid
#define WINDOW_FLG_FOCUSED  0x00000008 // has focus
#define WINDOW_FLG_CHECKED  0x00000010 // is checked/selected
#define WINDOW_FLG_CAPTURE  0x00000020 // capture jog events
#define WINDOW_FLG_DISABLED 0x00000040 // window is disabled (shadowed)
#define WINDOW_FLG_FREEMEM  0x00004000 // free memory when destroy
#define WINDOW_FLG_PARENT   0x00008000 // is parent window
#define WINDOW_FLG_USER     0x00010000 // user flags (WINDOW_FLG_USER<<n)
//top 1 byte cannot be used
//flag is stored there

//window events
#define WINDOW_EVENT_BTN_DN   0x01 //button down
#define WINDOW_EVENT_BTN_UP   0x02 //button up
#define WINDOW_EVENT_ENC_DN   0x03 //encoder minus
#define WINDOW_EVENT_ENC_UP   0x04 //encoder plus
#define WINDOW_EVENT_FOCUS0   0x05 //focus lost
#define WINDOW_EVENT_FOCUS1   0x06 //focus set
#define WINDOW_EVENT_CAPT_0   0x07 //capture lost
#define WINDOW_EVENT_CAPT_1   0x08 //capture set
#define WINDOW_EVENT_CLICK    0x09 //clicked (tag > 0)
#define WINDOW_EVENT_CHANGE   0x0a //value/index changed (tag > 0)
#define WINDOW_EVENT_CHANGING 0x0b //value/index changing (tag > 0)
#define WINDOW_EVENT_LOOP     0x0c //gui loop (every 50ms)
#define WINDOW_EVENT_TIMER    0x0d //gui timer
#define WINDOW_EVENT_MESSAGE  0x0e //onStatusChange() message notification

typedef struct _window_t window_t;

typedef void(window_init_t)(void *window);
typedef void(window_done_t)(void *window);
typedef void(window_draw_t)(void *window);
typedef void(window_event_t)(void *window, uint8_t event, void *param);

typedef struct _window_list_t window_list_t;
typedef void(window_list_item_t)(window_list_t *pwindow_list,
    uint16_t index, const char **pptext, uint16_t *pid_icon);

typedef struct _window_class_t {
    int16_t cls_id;        // (2 bytes) window class id
    uint16_t size;         // (2 bytes) window structure size
    window_init_t *init;   // (4 bytes) done callback
    window_done_t *done;   // (4 bytes) done callback
    window_draw_t *draw;   // (4 bytes) draw callback
    window_event_t *event; // (4 bytes) event callback
} window_class_t;          // (20 bytes total)

typedef struct _window_t {
    window_class_t *cls; // (4 bytes) window class pointer
    int16_t id_parent;   // (2 bytes) parent window identifier (2bytes)
    int16_t id;          // (2 bytes) window identifier (2bytes)
    union {
        uint32_t flg; // (3 bytes) flags (visibility, invalid...),
            // (1 byte)  top byte is window tag (user defined id)
        struct
        {
            uint32_t f_visible : 1;  // WINDOW_FLG_VISIBLE  0x00000001
            uint32_t f_enabled : 1;  // WINDOW_FLG_ENABLED  0x00000002
            uint32_t f_invalid : 1;  // WINDOW_FLG_INVALID  0x00000004
            uint32_t f_focused : 1;  // WINDOW_FLG_FOCUSED  0x00000008
            uint32_t f_checked : 1;  // WINDOW_FLG_CHECKED  0x00000010
            uint32_t f_capture : 1;  // WINDOW_FLG_CAPTURE  0x00000020
            uint32_t f_disabled : 1; // WINDOW_FLG_DISABLED 0x00000040
            uint32_t f_reserv0 : 6;  // reserved 7 bits
            uint32_t f_freemem : 1;  // WINDOW_FLG_FREEMEM  0x00002000
            uint32_t f_timer : 1;    // WINDOW_FLG_TIMER    0x00004000
            uint32_t f_parent : 1;   // WINDOW_FLG_PARENT   0x00008000
            uint32_t f_user : 1;     // WINDOW_FLG_USER     0x00010000
            uint32_t f_reserv1 : 7;  // reserved 7 bits
            uint32_t f_tag : 8;      // (1 byte) window tag (user defined id)
        };
    };
    rect_ui16_t rect;      // (8 bytes) display rectangle
    window_event_t *event; // (4 bytes) event callback
} window_t;                // (24 bytes total)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern window_t *window_popup_ptr; //current popup window

extern window_t *window_focused_ptr; //current focused window

extern window_t *window_capture_ptr; //current capture window

extern window_t *window_ptr(int16_t id);

extern int16_t window_id(window_t *ptr);

extern int16_t window_register_class(window_class_t *cls);

extern int16_t window_create(int16_t cls_id, int16_t id_parent, rect_ui16_t rect);

extern int16_t window_create_ptr(int16_t cls_id, int16_t id_parent, rect_ui16_t rect, void *ptr);

extern void window_destroy(int16_t id);

extern void window_destroy_children(int16_t id);

extern int16_t window_focused(void);

extern int16_t window_capture(void);

extern int16_t window_parent(int16_t id);

extern int16_t window_prev(int16_t id);

extern int16_t window_next(int16_t id);

extern int16_t window_prev_enabled(int16_t id);

extern int16_t window_next_enabled(int16_t id);

extern int16_t window_first_child(int16_t id);

extern int window_child_count(int16_t id);

extern int window_enabled_child_count(int16_t id);

extern int window_is_visible(int16_t id);

extern int window_is_enabled(int16_t id);

extern int window_is_invalid(int16_t id);

extern int window_is_focused(int16_t id);

extern int window_is_capture(int16_t id);

extern void window_draw(int16_t id);

extern void window_draw_children(int16_t id);

extern void window_validate(int16_t id);

extern void window_invalidate(int16_t id);

extern void window_validate_children(int16_t id);

extern void window_invalidate_children(int16_t id);

extern void window_set_tag(int16_t id, uint8_t tag);

extern void _window_set_tag(window_t *wnd, uint8_t tg);

extern uint8_t window_get_tag(int16_t id);

extern int _window_get_tag(window_t *wnd);

extern void window_set_text(int16_t id, const char *text);

extern char *window_get_text(int16_t id);

extern void window_set_value(int16_t id, float value);

extern float window_get_value(int16_t id);

extern void window_set_format(int16_t id, const char *format);

extern const char *window_get_format(int16_t id);

extern void window_set_color_back(int16_t id, color_t clr);

extern color_t window_get_color_back(int16_t id);

extern void window_set_color_text(int16_t id, color_t clr);

extern color_t window_get_color_text(int16_t id);

extern void window_set_focus(int16_t id);

extern void window_set_capture(int16_t id);

extern void window_enable(int16_t id);

extern void window_disable(int16_t id);

extern void window_show(int16_t id);

extern void window_hide(int16_t id);

extern void window_set_padding(int16_t id, padding_ui8_t padding);

extern void window_set_alignment(int16_t id, uint8_t alignment);

extern void window_set_item_count(int16_t id, int count);

extern int window_get_item_count(int16_t id);

extern void window_set_item_index(int16_t id, int index);

extern int window_get_item_index(int16_t id);

extern void window_set_top_index(int16_t id, int index);

extern int window_get_top_index(int16_t id);

extern void window_set_icon_id(int16_t id, uint16_t id_res);

extern uint16_t window_get_icon_id(int16_t id);

extern void window_set_min(int16_t id, float min);

extern float window_get_min(int16_t id);

extern void window_set_max(int16_t id, float max);

extern float window_get_max(int16_t id);

extern void window_set_step(int16_t id, float step);

extern float window_get_step(int16_t id);

extern void window_set_min_max(int16_t id, float min, float max);

extern void window_set_min_max_step(int16_t id, float min, float max, float step);

extern void window_dispatch_event(window_t *window, uint8_t event, void *param);

extern void window_set_item_callback(int16_t id, window_list_item_t *fnc);

extern void gui_invalidate(void);

#ifdef __cplusplus
}
#endif //__cplusplus

static inline void _window_invalidate(window_t *window) {
    if (!window)
        return;

    window->flg |= WINDOW_FLG_INVALID;
    gui_invalidate();
}

#endif //_WINDOW_H
