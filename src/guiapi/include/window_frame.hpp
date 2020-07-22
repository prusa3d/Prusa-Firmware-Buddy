// window_frame.hpp

#pragma once

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
    window_frame_t(window_t *first, window_t *parent = nullptr, rect_ui16_t rect = rect_ui16(0, 0, display::GetW(), display::GetH()));
    window_frame_t(rect_ui16_t rect, window_t *first); //Dialog ctor
    window_t *GetNextSubWin(window_t *win) const;
    window_t *GetPrevSubWin(window_t *win) const;
    window_t *GetNextEnabledSubWin(window_t *win) const;
    window_t *GetPrevEnabledSubWin(window_t *win) const;

protected:
    virtual void draw() override;
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void screenEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void invalidate(rect_ui16_t validation_rect = { 0 }) override;
    virtual void validate(rect_ui16_t validation_rect = { 0 }) override;
};
