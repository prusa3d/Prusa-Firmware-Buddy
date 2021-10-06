// window_temp_graph.hpp

#pragma once
#include "window.hpp"

struct window_temp_graph_t : public AddSuperWindow<window_t> {
    color_t color_extruder_t;
    color_t color_bed_t;
    color_t color_extruder_c;
    color_t color_bed_c;
    float y_min;
    float y_max;
    uint8_t count;
    uint8_t y_nozzle_t[180];
    uint8_t y_bed_t[180];
    uint8_t y_nozzle_c[180];
    uint8_t y_bed_c[180];
    bool graph_invalid;

    window_temp_graph_t(window_t *parent, Rect16 rect);

protected:
    virtual void unconditionalDraw() override;

    void redraw_point(uint16_t x, uint16_t y, uint8_t *data, color_t bg, color_t fg);
    void redraw_last_point(uint16_t x, uint16_t y0, uint16_t y1, color_t bg, color_t fg);
    void draw_axes(bool wipe_before_draw, bool xy_only);
};
