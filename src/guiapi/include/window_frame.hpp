// window_frame.hpp

#pragma once

#include "guitypes.hpp"
#include "GuiDefaults.hpp"
#include "window.hpp"
#include "display.h"

struct window_frame_t : public AddSuperWindow<window_t> {
    window_t *first;
    window_t *last;
    virtual void RegisterSubWin(window_t *win) override;
    virtual void UnregisterSubWin(window_t *win) override;
    window_t *GetFirst() const;
    window_t *GetLast() const;

    window_frame_t(window_t *parent = nullptr, Rect16 rect = GuiDefaults::RectScreen, is_dialog_t dialog = is_dialog_t::no, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_serial_t serial = is_closed_on_serial_t::yes);

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

    Rect16 GenerateRect(ShiftDir_t dir);

protected:
    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    virtual void invalidate(Rect16 validation_rect = Rect16()) override;
    virtual void validate(Rect16 validation_rect = Rect16()) override;
};
