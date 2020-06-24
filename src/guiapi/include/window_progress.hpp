// window_progress.hpp

#pragma once

#include "window_numb.hpp"

struct window_class_progress_t {
    window_class_t cls;
};

struct window_progress_t : public window_numb_t {
    color_t color_progress;
    uint8_t height_progress;
    float min;
    float max;

protected:
    virtual void setValue(float val) override;
};

extern const window_class_progress_t window_class_progress;
