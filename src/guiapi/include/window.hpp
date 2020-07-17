//window.hpp
#pragma once

#include <inttypes.h>
#include "guitypes.h"
#include <array>
#include "../../lang/string_view_utf8.hpp"

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

#define WINDOW_FLG_USER 0x00010000 // user flags (WINDOW_FLG_USER<<n)
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

struct window_list_t;
typedef void(window_list_item_t)(window_list_t *pwindow_list,
    uint16_t index, const char **pptext, uint16_t *pid_icon);

class window_t {
    window_t *parent;
    window_t *next;
    uint8_t f_tag;

protected:
    union {
        uint8_t flg;
        struct {
            bool f_visible : 1;
            bool f_enabled : 1; //  can be focussed
            bool f_invalid : 1;
            bool f_focused : 1;
            bool f_checked : 1;
            bool f_capture : 1;         //
            bool f_timer : 1;           // window has timers
            bool f_parent_defined0 : 1; //
        };
    };

public:
    int16_t id; // (2 bytes) window identifier (2bytes)

    rect_ui16_t rect; // (8 bytes) display rectangle
    color_t color_back;

    void SetNext(window_t *nxt);
    void SetParent(window_t *par);
    window_t *GetNext() const;
    window_t *GetNextEnabled() const;
    window_t *GetParent() const;
    void Draw();
    void DispatchEvent(window_t *sender, uint8_t ev, void *param); //try to handle, frame resends childern
    void Event(window_t *sender, uint8_t event, void *param);      //try to handle, send to parrent if not handled
    void ScreenEvent(window_t *sender, uint8_t event, void *param);
    bool IsVisible() const;
    bool IsEnabled() const;
    bool IsInvalid() const;
    bool IsFocused() const;
    bool IsCapture() const;
    bool HasTimer() const;
    void Validate();
    void Invalidate();
    void SetTag(uint8_t tag);
    uint8_t GetTag() const;

    void SetHasTimer();
    void ClrHasTimer();
    void SetFocus();
    void SetCapture();
    void Enable();
    void Disable();
    void Show();
    void Hide();
    void SetBackColor(color_t clr);
    color_t GetBackColor() const;

    window_t(window_t *parent = nullptr, window_t *prev = nullptr, rect_ui16_t rect = { 0 }); //todo remove nullptr default values
    virtual ~window_t();

protected:
    virtual void unconditionalDraw();
    virtual void draw();
    virtual int event(window_t *sender, uint8_t event, void *param) { return 0; }
    virtual void dispatchEvent(window_t *sender, uint8_t event, void *param);
};

extern window_t *window_popup_ptr; //current popup window

extern window_t *window_focused_ptr; //current focused window

extern window_t *window_capture_ptr; //current capture window

extern window_t *window_ptr(int16_t id);

extern int16_t window_create_ptr(int16_t cls_id, int16_t id_parent, rect_ui16_t rect, void *ptr);

extern void window_destroy(int16_t id);

extern int16_t window_focused(void);

extern int16_t window_capture(void);

extern void gui_invalidate(void);
