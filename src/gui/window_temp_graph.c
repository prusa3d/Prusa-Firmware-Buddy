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
}

void window_temp_graph_draw(window_temp_graph_t *window) {
    if (window->win.flg & WINDOW_FLG_INVALID) {
        rect_ui16_t rc = window->win.rect;
        const uint16_t x = x;
        const uint16_t y = window->win.rect.y;
        const uint16_t w = window->win.rect.w;
        const uint16_t h = window->win.rect.h;

        display->fill_rect(rc, window->color_back);
        display->draw_line(point_ui16(x, y - 1), point_ui16(x, y + h - 1), COLOR_WHITE); //hotend
        display->draw_line(point_ui16(x, (y + h) - 1), point_ui16((x + w), (y + h) - 1), COLOR_WHITE); //x

        uint8_t j;
        for (j = 25; j < 175; j += 25)
            display->draw_line(point_ui16(x + 1, y + h - j), point_ui16(x + w - 1 - 5, y + h - j), COLOR_GRAY); //x

        for (j = 25; j < 175; j += 25)
            display->draw_line(point_ui16(x + j, y + h), point_ui16(x + j, y + h - 5), COLOR_WHITE); //-50

        window->win.flg &= ~WINDOW_FLG_INVALID;
    }

    marlin_vars_t *vars = marlin_vars();

    window->y_bed_c[0] = (uint8_t)179 - (vars->temp_bed / 2);
    window->y_nozzle_c[0] = (uint8_t)179 - (vars->temp_nozzle / 2);

    window->y_nozzle_t[0] = (uint8_t)179 - (vars->target_nozzle / 2);
    window->y_bed_t[0] = (uint8_t)179 - (vars->target_bed / 2);

    if (window->win.flg & WINDOW_FLG_GRAPH_INVALID) {
        uint8_t i;
        uint8_t j;
        uint8_t ynt = window->y_nozzle_t[0];
        uint8_t ybt = window->y_bed_t[0];
        uint8_t ync = window->y_nozzle_c[0];
        uint8_t ybc = window->y_bed_c[0];
        const uint16_t x = window->win.rect.x;
        const uint16_t y = window->win.rect.y;
        const uint16_t w = window->win.rect.w;
        const uint16_t h = window->win.rect.h;

        for (i = 0; i < 178; i++) {
            display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_t[i]), window->color_back);
            display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_t[i + 1]), window->color_extruder_t);
            window->y_nozzle_t[i] = window->y_nozzle_t[i + 1];

            display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_t[i]), window->color_back);
            display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_t[i + 1]), window->color_bed_t);
            window->y_bed_t[i] = window->y_bed_t[i + 1];

            display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_c[i]), window->color_back);
            display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_c[i + 1]), window->color_extruder_c);
            window->y_nozzle_c[i] = window->y_nozzle_c[i + 1];

            display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_c[i]), window->color_back);
            display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_c[i + 1]), window->color_bed_c);
            window->y_bed_c[i] = window->y_bed_c[i + 1];
        }

        display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_t[i]), window->color_back);
        window->y_nozzle_t[i] = ynt;
        display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_t[i]), window->color_extruder_t);

        display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_t[i]), window->color_back);
        window->y_bed_t[i] = ybt;
        display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_t[i]), window->color_bed_t);

        display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_c[i]), window->color_back);
        window->y_nozzle_c[i] = ync;
        display->set_pixel(point_ui16(x + i + 1, y + window->y_nozzle_c[i]), window->color_extruder_c);

        display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_c[i]), window->color_back);
        window->y_bed_c[i] = ybc;
        display->set_pixel(point_ui16(x + i + 1, y + window->y_bed_c[i]), window->color_bed_c);

        //draw y line
        display->draw_line(point_ui16(x, y - 1), point_ui16(x, y + h - 1), COLOR_WHITE); //hotend
        display->draw_line(point_ui16(x, y + h - 1), point_ui16(x + w, y + h - 1), COLOR_WHITE); //x

        for (j = 25; j < 175; j += 25) 
            display->draw_line(point_ui16(x + 1, y + h - j), point_ui16(x + w - 1, y + h - j), COLOR_GRAY); //x

        for (j = 25; j < 175; j += 25)
            display->draw_line(point_ui16(x + j, y + h), point_ui16(x + j, y + h - 5), COLOR_WHITE); //-50

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
