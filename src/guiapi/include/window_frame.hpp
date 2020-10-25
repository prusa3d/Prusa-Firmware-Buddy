// window_frame.hpp

#pragma once

#include "guitypes.hpp"
#include "GuiDefaults.hpp"
#include "window.hpp"
#include "display.h"

//inherit, use ctor ti pass additional param
class WinFilter {
public:
    virtual bool operator()(const window_t &) const = 0;
};

class WinFilterTrue : public WinFilter {
public:
    virtual bool operator()(const window_t &) const override { return true; };
};

class WinFilterContained : public WinFilter {
    Rect16 rect;

public:
    constexpr WinFilterContained(Rect16 rc)
        : rect(rc) {}
    virtual bool operator()(const window_t &win) const override {
        return rect.Contain(win.rect);
    }
};

class window_frame_t : public AddSuperWindow<window_t> {
    window_t *first;
    window_t *last;

    window_t *findFirst(window_t *begin, window_t *end, const WinFilter &filter) const;
    window_t *findLast(window_t *begin, window_t *end, const WinFilter &filter) const;

    // this methods does not check rect or window type of win
    // public methods RegisterSubWin/UnregisterSubWin does
    // reference is used so nullptr test can be skipped
    void registerNormal(window_t &win);       // just register no need to check anything
    void registerDialog(window_t &win);       // register on top of all windows except strong_dialogs
    void registerStrongDialog(window_t &win); // just register no need to check anything
    void registerPopUp(window_t &win);        // fails if there is an overlaping dialog
    void unregisterNormal(window_t &win);     // does not do anything, unregistration is not needed for normal windows
    void unregisterDialog(window_t &win);
    void unregisterStrongDialog(window_t &win);
    void unregisterPopUp(window_t &win);

    window_t *getFirstOverlapingDialog(Rect16 intersection_rect) const;
    window_t *getFirstOverlapingPopUp(Rect16 intersection_rect) const;

public:
    virtual void RegisterSubWin(window_t *win) override;
    virtual void UnregisterSubWin(window_t *win) override;
    window_t *GetFirst() const;
    window_t *GetLast() const;

    window_frame_t(window_t *parent = nullptr, Rect16 rect = GuiDefaults::RectScreen, win_type_t type = win_type_t::normal, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_serial_t serial = is_closed_on_serial_t::yes);

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

private:
    virtual void invalidate(Rect16 validation_rect = Rect16()) override;
    virtual void validate(Rect16 validation_rect = Rect16()) override;
};
