// window_progress.hpp

#pragma once

#include "window_frame.hpp"

class window_numberless_progress_t : public AddSuperWindow<window_t> {
    color_t color_progress;
    int corner_radius; //< radius of rounded corner

protected:
    uint16_t progress_in_pixels;
    virtual void unconditionalDraw() override;

public:
    window_numberless_progress_t(window_t *parent, Rect16 rect, color_t cl_progress = COLOR_LIME, color_t cl_back = COLOR_GRAY, int corner_radius = 0);

    void SetProgressInPixels(uint16_t px);
    void SetProgressPercent(float val);
    uint16_t GetProgressPixels() const;

    void SetColor(color_t clr);
};

class window_vertical_progress_t : public AddSuperWindow<window_t> {
    color_t color_progress;
    uint16_t progress_in_pixels;

protected:
    virtual void unconditionalDraw() override;

public:
    window_vertical_progress_t(window_t *parent, Rect16 rect, color_t cl_progress = COLOR_ORANGE, color_t cl_back = COLOR_DARK_GRAY);
    void SetValue(float val);
    void SetProgressColor(color_t clr);
    void SetProgressWidth(uint16_t width);
    void SetProgressInPixels(uint16_t px);
    void SetProgressPercent(uint8_t val);
    uint16_t GetProgressPixels() const;
};

/**
 * @brief Draws number of circles with one current_index. All circles <= current index (or only current index if specified) have their color as "ON" ('progressed'), whilst all circles > current_index have the 'off' color (not yet done).
 * Circles always have diameter of given Rect16.Height(), so make sure the Rect is wide enough to hold all circles (there is an assert).
 */
class WindowProgressCircles : public AddSuperWindow<window_t> {
public:
    WindowProgressCircles(window_t *parent, Rect16 rect, uint8_t max_circles);
    [[nodiscard]] uint8_t get_current_index() const {
        return current_index;
    }
    [[nodiscard]] uint8_t get_max_circles() const {
        return max_circles;
    }
    void set_index(uint8_t new_index);
    void set_on_color(color_t clr);
    void set_off_color(color_t clr);
    void set_one_circle_mode(bool new_mode);

protected:
    void unconditionalDraw() override;

private:
    const uint8_t max_circles; // how many circles should be drawn within the rect, must be >0
    uint8_t current_index { 0 }; // current progress in range [0, max_circles)
    bool one_circle_mode { false }; // true if only current_index circle should be colored as 'on', false if current_index and also all previous should be colored as 'on'
    color_t color_on { COLOR_WHITE }; // color of circle that's ON
    color_t color_off { COLOR_GRAY }; // color of circle that's OFF
};
