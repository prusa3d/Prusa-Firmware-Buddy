// window_graph_y.h

#pragma once
#include "window.h"

#define WINDOW_FLG_GRAPH_INVALID (WINDOW_FLG_USER << 0)
typedef struct _window_temp_graph_t window_temp_graph_t;

typedef void(window_temp_graph_point_t)(window_temp_graph_t *pwindow_graph, uint8_t index, float y_val);

extern int16_t WINDOW_CLS_TEMP_GRAPH;

typedef struct _window_temp_graph_t {
    window_t win;
    color_t color_back;
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
} window_temp_graph_t;

typedef struct _window_class_temp_graph_t {
    window_class_t cls;
} window_class_temp_graph_t;

extern const window_class_temp_graph_t window_class_temp_graph;
