//window.hpp
#pragma once

#include <inttypes.h>
#include "guitypes.hpp"
#include "../../lang/string_view_utf8.hpp"
#include "Rect16.h"

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

//to be safe, ctor has this 2 bool parameters, can't switch them
enum class is_dialog_t : bool { no,
    yes };
enum class is_closed_on_click_t : bool { no,
    yes };

class window_t {
    window_t *parent;
    window_t *next;

protected:
    //todo add can capture flag (needed in frame event and SetCapture)
    union {
        uint16_t flg;
        struct {
            bool flag_visible : 1;                        // 00 - is visible
            bool flag_enabled : 1;                        // 01 - is enabled (can be focused)
            bool flag_invalid : 1;                        // 02 - content is invalid (draw)
            bool flag_checked : 1;                        // 03 - is checked/selected
            bool flag_timer : 1;                          // 04 - window has timers
            is_dialog_t flag_dialog : 1;                  // 05 - window id dialog
            is_closed_on_click_t flag_close_on_click : 1; // 06 - window id dialog
            bool flag_custom0 : 1;                        // 07 - this flag can be defined in parent
            bool flag_custom1 : 1;                        // 08 - this flag can be defined in parent
            bool flag_custom2 : 1;                        // 09 - this flag can be defined in parent
            bool flag_custom3 : 1;                        // 0A - this flag can be defined in parent
            bool flag_custom4 : 1;                        // 0B - this flag can be defined in parent
            bool flag_custom5 : 1;                        // 0C - this flag can be defined in parent
            bool flag_custom6 : 1;                        // 0D - this flag can be defined in parent
            bool flag_custom7 : 1;                        // 0E - this flag can be defined in parent
            bool flag_custom8 : 1;                        // 0F - this flag can be defined in parent
        };
    };

public:
    Rect16 rect; // (8 bytes) display rectangle
    color_t color_back;

public:
    void SetNext(window_t *nxt);
    void SetParent(window_t *par);
    window_t *GetNext() const;
    window_t *GetNextEnabled() const;
    window_t *GetParent() const;
    void Draw();
    void ScreenEvent(window_t *sender, uint8_t event, void *param); //try to handle, frame resends children
    void WindowEvent(window_t *sender, uint8_t event, void *param); //try to handle, can sent click to parent
    bool IsVisible() const;
    bool IsEnabled() const;
    bool IsInvalid() const;
    bool IsFocused() const;
    bool IsCaptured() const;
    bool HasTimer() const;
    bool IsDialog() const;
    void Validate(Rect16 validation_rect = Rect16());
    void Invalidate(Rect16 validation_rect = Rect16());

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

    window_t(window_t *parent, Rect16 rect, is_dialog_t dialog = is_dialog_t::no, is_closed_on_click_t close = is_closed_on_click_t::no);
    virtual ~window_t();

    virtual void RegisterSubWin(window_t *win);
    virtual void UnregisterSubWin(window_t *win) {} //meant for dialogs, remove this window from frame
protected:
    virtual void unconditionalDraw();
    virtual void draw();
    virtual void windowEvent(window_t *sender, uint8_t event, void *param);
    virtual void screenEvent(window_t *sender, uint8_t event, void *param);
    virtual void invalidate(Rect16 validation_rect);
    virtual void validate(Rect16 validation_rect);

private:
    static window_t *focused_ptr; // has focus
    static window_t *capture_ptr; // capture jog events

public:
    static window_t *GetFocusedWindow();
    static window_t *GetCapturedWindow();
};

void gui_invalidate(void);
