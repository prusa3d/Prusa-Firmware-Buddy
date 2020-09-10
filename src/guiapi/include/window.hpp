//window.hpp
#pragma once

#include <inttypes.h>
#include "guitypes.hpp"
#include "../../lang/string_view_utf8.hpp"
#include "Rect16.h"

//window events
enum {
    WINDOW_EVENT_BTN_DN = 1,   //button down
    WINDOW_EVENT_BTN_UP,       //button up
    WINDOW_EVENT_ENC_DN,       //encoder minus
    WINDOW_EVENT_ENC_UP,       //encoder plus
    WINDOW_EVENT_FOCUS0,       //focus lost
    WINDOW_EVENT_FOCUS1,       //focus set
    WINDOW_EVENT_CAPT_0,       //capture lost
    WINDOW_EVENT_CAPT_1,       //capture set
    WINDOW_EVENT_CLICK,        //clicked (tag > 0)
    WINDOW_EVENT_DOUBLE_CLICK, // double-clicked
    WINDOW_EVENT_HOLD,         // held button
    WINDOW_EVENT_CHANGE,       //value/index changed (tag > 0)
    WINDOW_EVENT_CHANGING,     //value/index changing (tag > 0)
    WINDOW_EVENT_LOOP,         //gui loop (every 50ms)
    WINDOW_EVENT_TIMER,        //gui timer
    WINDOW_EVENT_MESSAGE,      //onStatusChange() message notification
};

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
        uint32_t flg;
        struct {
            bool flag_visible : 1;                        // 00 - is visible
            bool flag_enabled : 1;                        // 01 - is enabled (can be focused)
            bool flag_invalid : 1;                        // 02 - content is invalid (draw)
            bool flag_checked : 1;                        // 03 - is checked/selected
            bool flag_timer : 1;                          // 04 - window has timers
            is_dialog_t flag_dialog : 1;                  // 05 - window id dialog
            is_closed_on_click_t flag_close_on_click : 1; // 06 - window id dialog
            bool flag_hidden_behind_dialog : 1;           // 07 - there is an dialog over this window
            bool flag_custom0 : 1;                        // 08 - this flag can be defined in parent
            bool flag_custom1 : 1;                        // 09 - this flag can be defined in parent
            bool flag_custom2 : 1;                        // 0A - this flag can be defined in parent
            bool flag_custom3 : 1;                        // 0B - this flag can be defined in parent
            bool flag_custom4 : 1;                        // 0C - this flag can be defined in parent
            bool flag_custom5 : 1;                        // 0D - this flag can be defined in parent
            bool flag_custom6 : 1;                        // 0E - this flag can be defined in parent
            bool flag_custom7 : 1;                        // 0F - this flag can be defined in parent

            // here would be 2 unused Bytes (structure data alignment),
            // make them accessible to be used in child to save RAM
            union {
                uint16_t mem_space_u16;
                int16_t mem_space_s16;
                std::array<uint8_t, 2> mem_array_u08;
                std::array<int8_t, 2> mem_array_s08;
            };
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
    bool IsChildOf(window_t *win) const;
    void Draw();
    void ScreenEvent(window_t *sender, uint8_t event, void *param); //try to handle, frame resends children
    void WindowEvent(window_t *sender, uint8_t event, void *param); //try to handle, can sent click to parent
    bool IsVisible() const;                                         // visible and not hidden by dialog
    bool IsHiddenBehindDialog() const;
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
    void HideBehindDialog();
    void ShowAfterDialog();
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

protected:
    static void ResetCapturedWindow();
};

/*****************************************************************************/
//window_aligned_t
//uses window_t  mem_array_u08[0] to store alignment (saves RAM)
struct window_aligned_t : public window_t {
    window_aligned_t(window_t *parent, Rect16 rect, is_dialog_t dialog = is_dialog_t::no, is_closed_on_click_t close = is_closed_on_click_t::no);
    /// alignment constants are in guitypes.h
    uint8_t GetAlignment() const;
    void SetAlignment(uint8_t alignment);
};

void gui_invalidate(void);
