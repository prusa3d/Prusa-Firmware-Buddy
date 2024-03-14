// window.hpp
#pragma once

#include "window_types.hpp"
#include "GuiDefaults.hpp"
#include "Rect16.h"
#include "window_event.hpp"
#include "align.hpp"
#include "color_scheme.hpp"
#include "gui_time.hpp" // not needed here, but will save lot of includes
#include "compact_pointer.hpp"
// !!! all windows should use gui::GetTick() to access tick value!!!

class window_t {
    Rect16 rect; // (8 bytes) display rectangle
    CompactRAMPointer<window_t> parent;
    CompactRAMPointer<window_t> next;

protected:
    WindowFlags flags;

private:
    // depends on color_scheme_background flag
    // if enabled and set != nullptr
    //   window automatically draws differently when selected or shadowed
    union {
        color_t color_back;
        const color_scheme *pBackColorScheme;
    };

public:
    Rect16 GetRect() const;
    Rect16 GetRectWithoutTransformation() const;
    void SetRect(Rect16 rc); // does not transform
    void SetRectWithoutTransformation(Rect16 rc);
    Rect16 TransformRect(Rect16 rc) const; // just transforms given rect, calls parrents transform if this window is relative

    inline Rect16::Left_t Left() const { return GetRect().Left(); }
    inline Rect16::Top_t Top() const { return GetRect().Top(); }
    inline Rect16::Width_t Width() const { return GetRect().Width(); }
    inline Rect16::Height_t Height() const { return GetRect().Height(); }

    void Reposition(Rect16::Top_t top);
    void Reposition(Rect16::Left_t left);
    void Resize(Rect16::Height_t height);
    void Resize(Rect16::Width_t width);

    template <class T>
    constexpr void operator+=(T val) {
        SetRect(GetRect() += val);
    }
    template <class T>
    constexpr void operator-=(T val) {
        SetRect(GetRect() -= val);
    }

    void SetNext(window_t *nxt);
    void SetParent(window_t *par);
    window_t *GetNext() const;
    window_t *GetNextEnabled() const;
    window_t *GetParent() const;
    bool IsChildOf(window_t *win) const;
    void Draw();
    void ScreenEvent(window_t *sender, GUI_event_t event, void *const param); // try to handle, frame resends children
    void WindowEvent(window_t *sender, GUI_event_t event, void *const param); // try to handle, can sent click to parent
    bool IsVisible() const; // visible and not hidden by dialog
    bool HasVisibleFlag() const; // visible, but still can be hidden behind dialog
    bool IsHiddenBehindDialog() const;
    bool IsEnabled() const;
    bool IsInvalid() const;
    bool HasValidBackground() const;
    bool IsFocused() const;
    bool IsCaptured() const;
    bool IsShadowed() const;
    bool IsCapturable() const;
    bool HasEnforcedCapture() const;
    bool HasTimer() const;
    bool HasIcon() const;
    win_type_t GetType() const;
    bool IsDialog() const;
    bool ClosedOnTimeout() const;
    bool ClosedOnPrint() const;
    void Validate(Rect16 validation_rect = Rect16());
    void Invalidate(Rect16 validation_rect = Rect16());
    void ValidateBackground(); // background cannot be invalidated alone, only validated

    void SetEnforceCapture();
    void ClrEnforceCapture();
    void EnableLongHoldScreenAction();
    void DisableLongHoldScreenAction();
    void SetHasTimer();
    void ClrHasTimer();
    void SetFocus();
    void Enable();
    void Disable();

    void set_visible(bool set);
    inline void Show() {
        set_visible(true);
    }
    inline void Hide() {
        set_visible(false);
    }

    void Shadow();
    void Unshadow();
    void HideBehindDialog();
    virtual void ShowAfterDialog();
    void SetBackColor(color_t clr);
    void SetBackColor(const color_scheme &clr);
    color_t GetBackColor() const;
    void SetRelativeSubwins() { flags.has_relative_subwins = true; }
    void SetRoundCorners() { flags.has_round_corners = true; }
    void SetHasIcon();
    void ClrHasIcon();
    void SetRedLayout();
    void SetBlackLayout();
    void SetBlueLayout();
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
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *const param);
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *const param);

    virtual bool registerSubWin(window_t &win);
    virtual void unregisterSubWin(window_t &win);
    virtual void addInvalidationRect(Rect16 rc);
    void notifyVisibilityChange();
    virtual void setRedLayout();
    virtual void setBlackLayout();
    virtual void setBlueLayout();

protected:
    virtual void invalidate(Rect16 validation_rect);
    virtual void validate(Rect16 validation_rect); // do not use, used by screen
    static window_t *focused_ptr; // has focus

public:
    virtual window_t *GetCapturedWindow();
    static window_t *GetFocusedWindow();
    static void ResetFocusedWindow();
};

// all children of window_t and their children must use AddSuperWindow<parent_window> for inheritance
template <class Base>
struct AddSuperWindow : public Base {
    template <class... Args>
    AddSuperWindow(Args &&...args)
        : Base(std::forward<Args>(args)...) {}

protected:
    typedef Base super;
    void SuperWindowEvent(window_t *sender, GUI_event_t event, void *const param) {
        static const char txt[] = "WindowEvent via super";
        super::windowEvent(EventLock(txt, sender, event), sender, event, param);
    }
};

/*****************************************************************************/
// window_aligned_t
// uses window_t flags to store alignment (saves RAM)
struct window_aligned_t : public AddSuperWindow<window_t> {
    window_aligned_t(window_t *parent, Rect16 rect, win_type_t type = win_type_t::normal, is_closed_on_click_t close = is_closed_on_click_t::no);
    /// alignment constants are in guitypes.hpp
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
