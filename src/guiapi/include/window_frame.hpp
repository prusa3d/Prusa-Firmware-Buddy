// window_frame.hpp

#pragma once

#include "guitypes.hpp"
#include "GuiDefaults.hpp"
#include "window.hpp"
#include "display.h"

struct window_frame_t : public window_t {
    //todo implement pointer to last
    window_t *first;
    window_t *last;
    virtual void RegisterSubWin(window_t *win) override;
    virtual void UnregisterSubWin(window_t *win) override;
    window_t *GetFirst() const;
    window_t *GetLast() const;
    window_frame_t(window_t *parent = nullptr, Rect16 rect = GuiDefaults::RectScreen, is_dialog_t dialog = is_dialog_t::no);
    window_t *GetNextSubWin(window_t *win, Rect16 rect = Rect16()) const;
    window_t *GetPrevSubWin(window_t *win, Rect16 rect = Rect16()) const;
    window_t *GetNextEnabledSubWin(window_t *win, Rect16 rect = Rect16()) const;
    window_t *GetPrevEnabledSubWin(window_t *win, Rect16 rect = Rect16()) const;
    window_t *GetFirstEnabledSubWin(Rect16 rect = Rect16()) const;
    bool IsChildCaptured();
    bool IsChildFocused();

protected:
    virtual void draw() override;
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void screenEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void invalidate(Rect16 validation_rect = Rect16()) override;
    virtual void validate(Rect16 validation_rect = Rect16()) override;
};
