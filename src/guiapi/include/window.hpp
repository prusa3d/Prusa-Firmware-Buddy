//window.hpp
#pragma once

#include <inttypes.h>
#include <functional>
#include "GuiDefaults.hpp"
#include "../../lang/string_view_utf8.hpp"
#include "Rect16.h"
#include "window_event.hpp"

using ButtonCallback = std::function<void()>;

struct window_list_t;
typedef void(window_list_item_t)(window_list_t *pwindow_list,
    uint16_t index, const char **pptext, uint16_t *pid_icon);

//to be safe, ctor has this 2 bool parameters, can't switch them
enum class is_closed_on_click_t : bool { no,
    yes };
enum class is_closed_on_timeout_t : bool { no,
    yes };
enum class is_closed_on_serial_t : bool { no,
    yes };

//type of window
//carefull if any states are added - flags and getter must be modified
enum class win_type_t {
    normal,       // normal window, registered in ctor, registration must succedd, does not unregister
    dialog,       // child of IDialog - modal window - multiple supported
    popup,        // similar to dialog, but does not claim capture, cannot overlap or be overlapped
                  //   by dialog (not registered / auto destroyed).
                  // destroyed when any window tries to overlap it
    strong_dialog // child of IDialog - stays on absolute top (normal dialog can open under it, but
                  //   will not get capture), only user can close it
                  // behavior of multipre strong dialogs is undefined for now
                  //   will be defined later - after we have multiple strong dialogs
};

class window_t {
    window_t *parent;
    window_t *next;

protected:
    //todo add can capture flag (needed in frame event and SetCapture)
    union {
        uint32_t flg;
        struct {
            uint32_t flags_type : 2;                       // 00 .. 01 - type of window
            bool flag_visible : 1;                         // 02 - is visible
            bool flag_enabled : 1;                         // 03 - is enabled (can be focused)
            bool flag_invalid : 1;                         // 04 - content is invalid (draw)
            bool flag_checked : 1;                         // 05 - is checked/selected
            bool flag_timer : 1;                           // 06 - window has timers
            is_closed_on_click_t flag_close_on_click : 1;  // 07 - window id dialog
            bool flag_hidden_behind_dialog : 1;            // 08 - there is an dialog over this window
            is_closed_on_timeout_t flag_timeout_close : 1; // 09 - menu timeout flag - it's meant to be used in window_frame_t
            is_closed_on_serial_t flag_serial_close : 1;   // 0A - serial printing screen open close
            bool flag_shadow : 1;                          // 0B - this flag can be defined in parent
            bool flag_custom0 : 1;                         // 0C - this flag can be defined in parent
            bool flag_custom1 : 1;                         // 0D - this flag can be defined in parent
            bool flag_custom2 : 1;                         // 0E - this flag can be defined in parent
            bool flag_custom3 : 1;                         // 0F - this flag can be defined in parent

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

    void SetNext(window_t *nxt);
    void SetParent(window_t *par);
    window_t *GetNext() const;
    window_t *GetNextEnabled() const;
    window_t *GetParent() const;
    bool IsChildOf(window_t *win) const;
    void Draw();
    void ScreenEvent(window_t *sender, GUI_event_t event, void *param); //try to handle, frame resends children
    void WindowEvent(window_t *sender, GUI_event_t event, void *param); //try to handle, can sent click to parent
    bool IsVisible() const;                                             // visible and not hidden by dialog
    bool IsHiddenBehindDialog() const;
    bool IsEnabled() const;
    bool IsInvalid() const;
    bool IsFocused() const;
    bool IsCaptured() const;
    bool IsShadowed() const;
    bool HasTimer() const;
    win_type_t GetType() const;
    bool IsDialog() const;
    bool ClosedOnTimeout() const;
    bool ClosedOnSerialPrint() const;
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
    void Shadow();
    void Unshadow();
    void HideBehindDialog();
    void ShowAfterDialog();
    void SetBackColor(color_t clr);
    color_t GetBackColor() const;

    window_t(window_t *parent, Rect16 rect, win_type_t type = win_type_t::normal, is_closed_on_click_t close = is_closed_on_click_t::no);
    virtual ~window_t();

    virtual bool RegisterSubWin(window_t *win);
    virtual void UnregisterSubWin(window_t *win) {} //meant for dialogs, remove this window from frame

    void ShiftNextTo(ShiftDir_t direction);
    virtual void Shift(ShiftDir_t direction, uint16_t distance);

protected:
    virtual void unconditionalDraw();
    virtual void draw();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *param);

private:
    virtual void invalidate(Rect16 validation_rect);
    virtual void validate(Rect16 validation_rect);

    static window_t *focused_ptr; // has focus
    static window_t *capture_ptr; // capture jog events

public:
    static window_t *GetFocusedWindow();
    static window_t *GetCapturedWindow();

    static void ResetCapturedWindow();
    static void ResetFocusedWindow();
};

//all children of window_t and their children must use AddSuperWindow<parent_window> for inheritance
template <class Base>
struct AddSuperWindow : public Base {
    template <class... T>
    AddSuperWindow(T... args)
        : Base(args...) {}

protected:
    typedef Base super;
    void SuperWindowEvent(window_t *sender, GUI_event_t event, void *param) {
        static const char txt[] = "WindowEvent via super";
        super::windowEvent(EventLock(txt, sender, event), sender, event, param);
    }
};

/*****************************************************************************/
//window_aligned_t
//uses window_t  mem_array_u08[0] to store alignment (saves RAM)
struct window_aligned_t : public AddSuperWindow<window_t> {
    window_aligned_t(window_t *parent, Rect16 rect, win_type_t type = win_type_t::normal, is_closed_on_click_t close = is_closed_on_click_t::no);
    /// alignment constants are in guitypes.h
    uint8_t GetAlignment() const;
    void SetAlignment(uint8_t alignment);
};

void gui_invalidate(void);
