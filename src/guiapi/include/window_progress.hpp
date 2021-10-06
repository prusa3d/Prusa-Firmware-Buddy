// window_progress.hpp

#pragma once

#include "window_numb.hpp"
#include "window_frame.hpp"

class window_numberless_progress_t : public AddSuperWindow<window_t> {
    color_t color_progress;
    uint16_t progress_in_pixels;
    virtual void unconditionalDraw() override;

public:
    window_numberless_progress_t(window_t *parent, Rect16 rect, color_t cl_progress = COLOR_LIME, color_t cl_back = COLOR_GRAY);

    void SetProgressInPixels(uint16_t px);
    void SetProgressPercent(float val);
    uint16_t GetProgressPixels() const;

    void SetColor(color_t clr);
};

class window_progress_t : public AddSuperWindow<window_frame_t> {
    window_numberless_progress_t progr;
    window_numb_t numb;
    float min;
    float max;

public:
    window_progress_t(window_t *parent, Rect16 rect, uint16_t h_progr = 8, color_t cl_progress = COLOR_LIME, color_t cl_back = COLOR_GRAY);
    void SetValue(float val);
    void SetFont(font_t *val);
    void SetProgressColor(color_t clr);
    void SetNumbColor(color_t clr);
    void SetProgressHeight(Rect16::Height_t height);
};
