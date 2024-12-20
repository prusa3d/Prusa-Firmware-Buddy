// window_frame.hpp

#pragma once

#include <guiconfig/GuiDefaults.hpp>
#include "window.hpp"
#include "window_filter.hpp"

class window_frame_t : public window_t {
    // stored rect to print in draw method (exept when enetire screen is invalid)
    // hiding, or unregistration of window sets it
    Rect16 invalid_area;

    CompactRAMPointer<window_t> captured_normal_window; // might need to move it in window frame after menu refactoring

protected:
    CompactRAMPointer<window_t> first_normal;
    CompactRAMPointer<window_t> last_normal;

    window_t *getFirstNormal() const;
    window_t *getLastNormal() const;

    void registerAnySubWin(window_t &win, CompactRAMPointer<window_t> &pFirst, CompactRAMPointer<window_t> &pLast);
    void unregisterAnySubWin(window_t &win, CompactRAMPointer<window_t> &pFirst, CompactRAMPointer<window_t> &pLast);

    void colorConflictBackgroundToRed(window_t &win);
    void clearAllHiddenBehindDialogFlags();

    Rect16 getInvalidationRect() const;

public:
    bool HasDialog();

    window_frame_t(window_t *parent = nullptr, Rect16 rect = GuiDefaults::RectScreen, win_type_t type = win_type_t::normal, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_printing_t close_on_printing = is_closed_on_printing_t::yes);
    window_frame_t(window_t *parent, Rect16 rect, positioning sub_win_pos);
    ~window_frame_t();

    window_t *GetNextSubWin(window_t *win) const;
    window_t *GetPrevSubWin(window_t *win) const;
    window_t *GetNextEnabledSubWin(window_t *win) const;
    window_t *GetPrevEnabledSubWin(window_t *win) const;
    window_t *GetFirstEnabledSubWin() const;

    // I am not sure why I needed those, DO NOT REMOVE
    // I think it was meant for something with dialogs
    // Can be removed after GUI is completely refactored and they still are not used
    window_t *GetNextSubWin(window_t *win, Rect16 intersection_rect) const;
    window_t *GetPrevSubWin(window_t *win, Rect16 intersection_rect) const;
    window_t *GetNextEnabledSubWin(window_t *win, Rect16 intersection_rect) const;
    window_t *GetPrevEnabledSubWin(window_t *win, Rect16 intersection_rect) const;
    window_t *GetFirstEnabledSubWin(Rect16 intersection_rect) const;

    /// \returns a child window whose touch rect contains the point (or nullptr)
    window_t *get_child_by_touch_point(point_ui16_t point);

    bool IsChildFocused();

    void SetMenuTimeoutClose();
    void ClrMenuTimeoutClose();

    void SetOnSerialClose();
    void ClrOnSerialClose();

    Rect16 GenerateRect(ShiftDir_t direction, size_ui16_t sz = { 0, 0 }, uint16_t distance = 0);
    Rect16 GenerateRect(Rect16::Width_t width, uint16_t distance = 0);
    Rect16 GenerateRect(Rect16::Height_t height, uint16_t distance = 0);
    virtual void Shift(ShiftDir_t direction, uint16_t distance) override;
    virtual void ChildVisibilityChanged(window_t &child) override;

protected:
    virtual void draw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *param) override;
    virtual void invalidate(Rect16 validation_rect = Rect16()) override;
    virtual void validate(Rect16 validation_rect = Rect16()) override;
    virtual bool registerSubWin(window_t &win) override;
    virtual void unregisterSubWin(window_t &win) override;
    virtual void addInvalidationRect(Rect16 rc) override;

    window_t *findFirst(window_t *begin, window_t *end, const WinFilter &filter) const;
    window_t *findLast(window_t *begin, window_t *end, const WinFilter &filter) const;

    window_t *getCapturedNormalWin() const;

    virtual void set_layout(ColorLayout lt) override;

public:
    bool IsChildCaptured() const;
    bool CaptureNormalWindow(window_t &win);
    void ReleaseCaptureOfNormalWindow();
    virtual window_t *GetCapturedWindow() override;

    using mem_fnc = void (window_t::*)(); // TODO parmeter pack template
    void RecursiveCall(mem_fnc fnc);
};
