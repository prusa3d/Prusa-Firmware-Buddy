// window_temp_graph.hpp

#pragma once
#include "window.hpp"

#define WINDOW_FLG_GRAPH_INVALID (WINDOW_FLG_USER << 0)
struct window_temp_graph_t;

typedef void(window_temp_graph_point_t)(window_temp_graph_t *pwindow_graph, uint8_t index, float y_val);

extern int16_t WINDOW_CLS_TEMP_GRAPH;

struct window_temp_graph_t : public window_t {
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
};

struct window_class_temp_graph_t {
    window_class_t cls;
};

extern const window_class_temp_graph_t window_class_temp_graph;
