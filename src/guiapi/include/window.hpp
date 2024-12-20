// window.hpp
#pragma once

#include "window_types.hpp"
#include <guiconfig/GuiDefaults.hpp>
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
        Color color_back = GuiDefaults::ColorBack;
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

    /// Returns rect for checking against touch events.
    /// Usually, this is same as the window native rectangle, but can be larger for example for radio buttons so that they're easier to click on
    virtual Rect16 get_rect_for_touch() const;

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

    bool IsInvalid() const;
    bool HasValidBackground() const;
    bool IsFocused() const;
    bool IsCaptured() const;
    bool IsCapturable() const;
    bool HasEnforcedCapture() const;
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
    void SetFocus();

    inline bool IsEnabled() const {
        return flags.enabled;
    }
    void set_enabled(bool set);
    inline void Enable() {
        set_enabled(true);
    }
    inline void Disable() {
        set_enabled(false);
    }

    void set_visible(bool set);
    inline void Show() {
        set_visible(true);
    }
    inline void Hide() {
        set_visible(false);
    }

    inline bool IsShadowed() const {
        return flags.shadow;
    }
    void set_shadow(bool set);
    inline void Shadow() { set_shadow(true); } /// Removeme ugly legacy function
    inline void Unshadow() { set_shadow(false); } /// Removeme ugly legacy function

    void HideBehindDialog();
    virtual void ShowAfterDialog();
    void SetBackColor(Color clr);
    void SetBackColor(const color_scheme &clr);
    Color GetBackColor() const;
    void SetRelativeSubwins() { flags.has_relative_subwins = true; }
    void SetRoundCorners() { flags.has_round_corners = true; }
    void SetHasIcon();
    void ClrHasIcon();

    void SetRedLayout();
    void SetBlackLayout();
    void SetBlueLayout();

    window_t() = default;
    window_t(window_t *parent, Rect16 rect, win_type_t type = win_type_t::normal, is_closed_on_click_t close = is_closed_on_click_t::no);

    bool RegisterSubWin(window_t &win);
    void UnregisterSubWin(window_t &win);

    void ShiftNextTo(ShiftDir_t direction);
    virtual void Shift(ShiftDir_t direction, uint16_t distance);
    virtual void ChildVisibilityChanged(window_t &child);

    enum class ChildDialogParam : uint8_t {
        first_dialog,
        last_dialog,
    };

    virtual window_t *get_child_dialog([[maybe_unused]] ChildDialogParam param) const {
        return nullptr;
    }

    inline window_t *GetFirstDialog() const { return get_child_dialog(ChildDialogParam::first_dialog); }
    inline window_t *GetLastDialog() const { return get_child_dialog(ChildDialogParam::last_dialog); }

protected:
    // Make the destructor protected to prevent accidentally calling this through a base class now that it's non-virtual (for flash saving reasons - BFW-5031)
    ~window_t();

    virtual void unconditionalDraw();
    virtual void draw();
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *const param);
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *const param);

    virtual bool registerSubWin(window_t &win);
    virtual void unregisterSubWin(window_t &win);
    virtual void addInvalidationRect(Rect16 rc);
    void notifyVisibilityChange();

    enum class ColorLayout : uint8_t {
        red,
        black,
        blue,
    };
    virtual void set_layout(ColorLayout set);

protected:
    virtual void invalidate(Rect16 validation_rect);
    virtual void validate(Rect16 validation_rect); // do not use, used by screen
    static window_t *focused_ptr; // has focus

public:
    virtual window_t *GetCapturedWindow();
    static window_t *GetFocusedWindow();
    static void ResetFocusedWindow();
};

/// Final variant of window_t, to get around the window_t protected destructor
class BasicWindow final : public window_t {
public:
    using window_t::window_t;
};

/*****************************************************************************/
// window_aligned_t
// uses window_t flags to store alignment (saves RAM)
class window_aligned_t : public window_t {

public:
    window_aligned_t() = default;
    window_aligned_t(window_t *parent, Rect16 rect, win_type_t type = win_type_t::normal, is_closed_on_click_t close = is_closed_on_click_t::no);

public:
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
