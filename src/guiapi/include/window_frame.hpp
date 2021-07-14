// window_frame.hpp

#pragma once

#include "window.hpp"
#include "window_filter.hpp"

class window_frame_t : public AddSuperWindow<window_t> {
    window_t *captured_normal_window; //might need to move it in window frame after menu refactoring

    // stored rect to print in draw method (exept when enetire screen is invalid)
    // hiding, or unregistration of window sets it
    Rect16 invalid_area;

protected:
    window_t *first_normal;
    window_t *last_normal;

    window_t *getFirstNormal() const;
    window_t *getLastNormal() const;

    void registerAnySubWin(window_t &win, window_t *&pFirst, window_t *&pLast);
    void unregisterAnySubWin(window_t &win, window_t *&pFirst, window_t *&pLast);

    void colorConflictBackgroundToRed(window_t &win);
    void clearAllHiddenBehindDialogFlags();

    Rect16 getInvalidationRect() const;

public:
    bool HasDialogOrPopup();

    window_frame_t(window_t *parent = nullptr, Rect16 rect = GuiDefaults::RectScreen, win_type_t type = win_type_t::normal, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_serial_t serial = is_closed_on_serial_t::yes);
    window_frame_t(window_t *parent, Rect16 rect, positioning sub_win_pos);
    virtual ~window_frame_t() override;
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

    bool IsChildFocused();

    void SetMenuTimeoutClose();
    void ClrMenuTimeoutClose();

    void SetOnSerialClose();
    void ClrOnSerialClose();

    Rect16 GenerateRect(ShiftDir_t direction);
    virtual void Shift(ShiftDir_t direction, uint16_t distance) override;
    virtual void ChildVisibilityChanged(window_t &child) override;

protected:
    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *param) override;
    virtual void invalidate(Rect16 validation_rect = Rect16()) override;
    virtual void validate(Rect16 validation_rect = Rect16()) override;
    virtual bool registerSubWin(window_t &win) override;
    virtual void unregisterSubWin(window_t &win) override;
    virtual void addInvalidationRect(Rect16 rc) override;

    window_t *findFirst(window_t *begin, window_t *end, const WinFilter &filter) const;
    window_t *findLast(window_t *begin, window_t *end, const WinFilter &filter) const;

    window_t *getCapturedNormalWin() const;

public:
    bool IsChildCaptured() const;
    bool CaptureNormalWindow(window_t &win);
    void ReleaseCaptureOfNormalWindow();
    virtual window_t *GetCapturedWindow() override;
};
