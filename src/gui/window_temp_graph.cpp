// window_graph_y.c
#include <window_temp_graph.h>
#include "display_helper.h"
#include "gui.h"
#include "marlin_client.h"
#include <stdlib.h>

int16_t WINDOW_CLS_TEMP_GRAPH = 0;

void window_temp_graph_init(window_temp_graph_t *window) {
    window->color_back = COLOR_BLACK;
    window->color_extruder_t = COLOR_LIME;
    window->color_bed_t = COLOR_CYAN;
    window->color_extruder_c = COLOR_ORANGE;
    window->color_bed_c = COLOR_BLUE;
    window->y_min = 0.0F;
    window->y_max = 300.0F;
    window->count = 200;
    uint8_t i;

    for (i = 0; i < 179; i++) {
        window->y_bed_c[i] = 179.0F;
        window->y_nozzle_c[i] = 179.0F;
        window->y_bed_t[i] = 179.0F;
        window->y_nozzle_t[i] = 179.0F;
    }

    display::FillRect(window->win.rect, window->color_back);
}

void redraw_point(uint16_t x, uint16_t y, uint8_t *data, color_t bg, color_t fg) {
    display::SetPixel(point_ui16(x, y + data[0]), bg);
    display::SetPixel(point_ui16(x, y + data[1]), fg);
}

void redraw_last_point(uint16_t x, uint16_t y0, uint16_t y1, color_t bg, color_t fg) {
    display::SetPixel(point_ui16(x, y0), bg);
    display::SetPixel(point_ui16(x, y1), fg);
}

void draw_axes(window_temp_graph_t *window, bool wipe_before_draw, bool xy_only) {
    const uint16_t x = window->win.rect.x;
    const uint16_t y = window->win.rect.y;
    const uint16_t w = window->win.rect.w;
    const uint16_t h = window->win.rect.h;

    if (wipe_before_draw)
        display::FillRect(window->win.rect, window->color_back);
    display::DrawLine(point_ui16(x, y - 1), point_ui16(x, y + h - 1), COLOR_WHITE);             //y
    display::DrawLine(point_ui16(x, y + h - 1), point_ui16(x + w - 1, y + h - 1), COLOR_WHITE); //x

    if (!xy_only) {
        uint8_t j;
        for (j = 25; j < 175; j += 25)
            display::DrawLine(point_ui16(x + 1, y + h - j), point_ui16(x + w - 1, y + h - j), COLOR_GRAY); //x
        for (j = 25; j < 175; j += 25)
            display::DrawLine(point_ui16(x + j, y + h), point_ui16(x + j, y + h - 5), COLOR_GRAY); //-50
    }
}

void window_temp_graph_draw(window_temp_graph_t *window) {
    const bool redraw_graph = window->win.flg & WINDOW_FLG_GRAPH_INVALID;
    if (!redraw_graph && window->win.flg & WINDOW_FLG_INVALID) {
        draw_axes(window, true, false);
        window->win.flg &= ~WINDOW_FLG_INVALID;
        return;
    }

    marlin_vars_t *vars = marlin_vars();

    window->y_bed_c[0] = (uint8_t)(179 - (vars->temp_bed * 0.5F));
    window->y_nozzle_c[0] = (uint8_t)(179 - (vars->temp_nozzle * 0.5F));
    window->y_nozzle_t[0] = (uint8_t)(179 - (vars->target_nozzle * 0.5F));
    window->y_bed_t[0] = (uint8_t)(179 - (vars->target_bed * 0.5F));

    if (redraw_graph) {
        draw_axes(window, false, false); //draw now not overwrite real temperatures

        uint8_t i;
        uint8_t ynt = window->y_nozzle_t[0];
        uint8_t ybt = window->y_bed_t[0];
        uint8_t ync = window->y_nozzle_c[0];
        uint8_t ybc = window->y_bed_c[0];

        const uint16_t x = window->win.rect.x;
        const uint16_t y = window->win.rect.y;

        for (i = 0; i < 178; i++) {
            redraw_point(x + i + 1, y, &window->y_nozzle_t[i], window->color_back, window->color_extruder_t);
            window->y_nozzle_t[i] = window->y_nozzle_t[i + 1];

            redraw_point(x + i + 1, y, &window->y_bed_t[i], window->color_back, window->color_bed_t);
            window->y_bed_t[i] = window->y_bed_t[i + 1];

            redraw_point(x + i + 1, y, &window->y_nozzle_c[i], window->color_back, window->color_extruder_c);
            window->y_nozzle_c[i] = window->y_nozzle_c[i + 1];

            redraw_point(x + i + 1, y, &window->y_bed_c[i], window->color_back, window->color_bed_c);
            window->y_bed_c[i] = window->y_bed_c[i + 1];
        }

        //FIXME leaves trace in the graph but bed temp. does not do it
        redraw_last_point(x + i + 1, y + window->y_nozzle_t[i], y + ynt, window->color_back, window->color_extruder_t);
        window->y_nozzle_t[i] = ynt;

        redraw_last_point(x + i + 1, y + window->y_bed_t[i], y + ybt, window->color_back, window->color_bed_t);
        window->y_bed_t[i] = ybt;

        redraw_last_point(x + i + 1, y + window->y_nozzle_c[i], y + ync, window->color_back, window->color_extruder_c);
        window->y_nozzle_c[i] = ync;

        redraw_last_point(x + i + 1, y + window->y_bed_c[i], y + ybc, window->color_back, window->color_bed_c);
        window->y_bed_c[i] = ybc;

        draw_axes(window, false, true); //hides 0 values
        window->win.flg &= ~WINDOW_FLG_GRAPH_INVALID;
    }
}

const window_class_temp_graph_t window_class_temp_graph = {
    {
        WINDOW_CLS_USER,
        sizeof(window_temp_graph_t),
        (window_init_t *)window_temp_graph_init,
        0,
        (window_init_t *)window_temp_graph_draw,
        0,
    },
};
