//window.hpp
#pragma once

#include <inttypes.h>
#include "guitypes.h"
#include <array>
#include "../../lang/string_view_utf8.hpp"

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

using ButtonCallback = void (*)();

struct window_list_t;
typedef void(window_list_item_t)(window_list_t *pwindow_list,
    uint16_t index, const char **pptext, uint16_t *pid_icon);

//to be safe ctor has this 2 bool parameters, can't switch them
enum class is_dialog_t : bool { no,
    yes };
enum class is_closed_on_click_t : bool { no,
    yes };

class window_t {
    window_t *parent;
    window_t *next;

protected:
    union {
        uint16_t flg;
        struct {
            bool f_visible : 1;                               // 00 - is visible
            bool f_enabled : 1;                               // 01 - is enabled (can be focused)
            bool f_invalid : 1;                               // 02 - content is invalid
            bool f_checked : 1;                               // 03 - is checked/selected
            bool f_timer : 1;                                 // 04 - window has timers
            is_dialog_t f_dialog : 1;                         // 05 - window id dialog
            is_closed_on_click_t f_on_click_close_screen : 1; // 06 - window id dialog
            bool f_parent_defined0 : 1;                       // 07 - this flag can be defined in parent
            bool f_parent_defined1 : 1;                       // 08 - this flag can be defined in parent
            bool f_parent_defined2 : 1;                       // 09 - this flag can be defined in parent
            bool f_parent_defined3 : 1;                       // 0A - this flag can be defined in parent
            bool f_parent_defined4 : 1;                       // 0B - this flag can be defined in parent
            bool f_parent_defined5 : 1;                       // 0C - this flag can be defined in parent
            bool f_parent_defined6 : 1;                       // 0D - this flag can be defined in parent
            bool f_parent_defined7 : 1;                       // 0E - this flag can be defined in parent
            bool f_parent_defined8 : 1;                       // 0F - this flag can be defined in parent
        };
    };

public:
    rect_ui16_t rect; // (8 bytes) display rectangle
    color_t color_back;

public:
    void SetNext(window_t *nxt);
    void SetParent(window_t *par);
    window_t *GetNext() const;
    window_t *GetNextEnabled() const;
    window_t *GetParent() const;
    void Draw();
    void ScreenEvent(window_t *sender, uint8_t event, void *param); //try to handle, frame resends childern
    void WindowEvent(window_t *sender, uint8_t event, void *param); //try to handle, can sent click to parent
    bool IsVisible() const;
    bool IsEnabled() const;
    bool IsInvalid() const;
    bool IsFocused() const;
    bool IsCapture() const;
    bool HasTimer() const;
    bool IsDialog() const;
    void Validate(rect_ui16_t validation_rect = { 0 });
    void Invalidate(rect_ui16_t validation_rect = { 0 });

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

    window_t(window_t *parent, rect_ui16_t rect, is_dialog_t dialog = is_dialog_t::no);
    virtual ~window_t();

    virtual void RegisterSubWin(window_t *win);
    virtual void UnregisterSubWin(window_t *win) {} //meant for dialogs, remove this window from frame
protected:
    virtual void unconditionalDraw();
    virtual void draw();
    virtual void windowEvent(window_t *sender, uint8_t event, void *param);
    virtual void screenEvent(window_t *sender, uint8_t event, void *param);
    virtual void invalidate(rect_ui16_t validation_rect);
    virtual void validate(rect_ui16_t validation_rect);

private:
    static window_t *focused_ptr; // has focus
    static window_t *capture_ptr; // capture jog events

public:
    static window_t *GetFocusedWindow();
    static window_t *GetCapturedWindow();
};

void gui_invalidate(void);
