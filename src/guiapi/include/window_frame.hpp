// window_frame.hpp

#pragma once

#include "window.hpp"
#include "window_filter.hpp"

class window_frame_t : public AddSuperWindow<window_t> {
    window_t *first;
    window_t *last;

    virtual void invalidate(Rect16 validation_rect = Rect16()) override;
    virtual void validate(Rect16 validation_rect = Rect16()) override;

    // this methods does not check rect or window type of win
    // public methods RegisterSubWin/UnregisterSubWin does
    // reference is used so nullptr test can be skipped
    bool registerNormal(window_t &win);         // just register no need to check anything
    bool registerDialog(window_t &win);         // register on top of all windows except strong_dialogs
    bool registerStrongDialog(window_t &win);   // just register no need to check anything
    bool registerPopUp(window_t &win);          // fails if there is an overlaping dialog
    void unregisterNormal(window_t &win);       // normal unregistration
    void unregisterDialog(window_t &win);       // normal unregistration, manage hidden behind dialog flags
    void unregisterStrongDialog(window_t &win); // normal unregistration, todo what if there is more than one strong dialog?
    void unregisterPopUp(window_t &win);        // just notify popup about unregistration, it wil unregister itself

public:
    virtual bool RegisterSubWin(window_t *win) override;
    virtual void UnregisterSubWin(window_t *win) override;
    window_t *GetFirst() const;
    window_t *GetLast() const;

    window_frame_t(window_t *parent = nullptr, Rect16 rect = GuiDefaults::RectScreen, win_type_t type = win_type_t::normal, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_serial_t serial = is_closed_on_serial_t::yes);
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

    bool IsChildCaptured();
    bool IsChildFocused();

    void SetMenuTimeoutClose();
    void ClrMenuTimeoutClose();

    void SetOnSerialClose();
    void ClrOnSerialClose();

    Rect16 GenerateRect(ShiftDir_t direction);
    virtual void Shift(ShiftDir_t direction, uint16_t distance) override;

protected:
    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

    window_t *findFirst(window_t *begin, window_t *end, const WinFilter &filter) const;
    window_t *findLast(window_t *begin, window_t *end, const WinFilter &filter) const;
};
