//window.hpp
#pragma once

#include "window_types.hpp"
#include "GuiDefaults.hpp"
#include "Rect16.h"
#include "window_event.hpp"
#include "align.hpp"

class window_t {
    window_t *parent;
    window_t *next;

protected:
    WindowFlags flags;

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
    bool HasVisibleFlag() const;                                        // visible, but still can be hidden behind dialog
    bool IsHiddenBehindDialog() const;
    bool IsEnabled() const;
    bool IsInvalid() const;
    bool IsFocused() const;
    bool IsCaptured() const;
    bool IsShadowed() const;
    bool IsCapturable() const;
    bool HasEnforcedCapture() const;
    bool HasTimer() const;
    win_type_t GetType() const;
    bool IsDialog() const;
    bool ClosedOnTimeout() const;
    bool ClosedOnSerialPrint() const;
    void Validate(Rect16 validation_rect = Rect16());
    void Invalidate(Rect16 validation_rect = Rect16());

    void SetEnforceCapture();
    void ClrEnforceCapture();
    void SetHasTimer();
    void ClrHasTimer();
    void SetFocus();
    void Enable();
    void Disable();
    void Show();
    void Hide();
    void Shadow();
    void Unshadow();
    void HideBehindDialog();
    virtual void ShowAfterDialog();
    void SetBackColor(color_t clr);
    color_t GetBackColor() const;

    window_t(window_t *parent, Rect16 rect, win_type_t type = win_type_t::normal, is_closed_on_click_t close = is_closed_on_click_t::no);
    virtual ~window_t();

    bool RegisterSubWin(window_t &win);
    void UnregisterSubWin(window_t &win);

    void ShiftNextTo(ShiftDir_t direction);
    virtual void Shift(ShiftDir_t direction, uint16_t distance);
    virtual void ChildVisibilityChanged(window_t &child);

    virtual window_t *GetFirstDialog() const { return nullptr; }
    virtual window_t *GetLastDialog() const { return nullptr; }

    virtual window_t *GetFirstStrongDialog() const { return nullptr; }
    virtual window_t *GetLastStrongDialog() const { return nullptr; }

    virtual window_t *GetFirstPopUp() const { return nullptr; }
    virtual window_t *GetLastPopUp() const { return nullptr; }

protected:
    virtual void unconditionalDraw();
    virtual void draw();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *param);

    virtual bool registerSubWin(window_t &win);
    virtual void unregisterSubWin(window_t &win);
    virtual void addInvalidationRect(Rect16 rc);
    void notifyVisibilityChange();

private:
    virtual void invalidate(Rect16 validation_rect);
    virtual void validate(Rect16 validation_rect); // do not use, used by screen

    static window_t *focused_ptr; // has focus

public:
    virtual window_t *GetCapturedWindow();
    static window_t *GetFocusedWindow();
    static void ResetFocusedWindow();

    //knob events
    static bool EventEncoder(int diff);
    static bool EventJogwheel(BtnState_t state);
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
    Align_t GetAlignment() const;
    void SetAlignment(Align_t alignment);
};

class DoNotEnforceCapture_ScopeLock {
    window_t &ths;
    bool enforce;

public:
    DoNotEnforceCapture_ScopeLock(window_t &win)
        : ths(win)
        , enforce(win.HasEnforcedCapture()) {
        ths.ClrEnforceCapture();
    }
    ~DoNotEnforceCapture_ScopeLock() {
        enforce ? ths.SetEnforceCapture() : ths.ClrEnforceCapture();
    }
};

void gui_invalidate(void);
