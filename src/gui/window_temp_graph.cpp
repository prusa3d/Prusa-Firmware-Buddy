// window_temp_graph.cpp
#include <window_temp_graph.hpp>
#include "display_helper.h"
#include "gui.hpp"
#include "marlin_client.h"
#include <stdlib.h>

window_temp_graph_t::window_temp_graph_t(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_t>(parent, rect)
    , color_extruder_t(COLOR_LIME)
    , color_bed_t(COLOR_CYAN)
    , color_extruder_c(COLOR_ORANGE)
    , color_bed_c(COLOR_BLUE)
    , y_min(0.0F)
    , y_max(300.0F)
    , count(200)
    , graph_invalid(true) {

    uint8_t i;

    for (i = 0; i < 179; i++) {
        y_bed_c[i] = 179.0F;
        y_nozzle_c[i] = 179.0F;
        y_bed_t[i] = 179.0F;
        y_nozzle_t[i] = 179.0F;
    }
}

void window_temp_graph_t::redraw_point(uint16_t x, uint16_t y, uint8_t *data, color_t bg, color_t fg) {
    display::SetPixel(point_ui16(x, y + data[0]), bg);
    display::SetPixel(point_ui16(x, y + data[1]), fg);
}

void window_temp_graph_t::redraw_last_point(uint16_t x, uint16_t y0, uint16_t y1, color_t bg, color_t fg) {
    display::SetPixel(point_ui16(x, y0), bg);
    display::SetPixel(point_ui16(x, y1), fg);
}

void window_temp_graph_t::draw_axes(bool wipe_before_draw, bool xy_only) {
    const uint16_t x = Left();
    const uint16_t y = Top();
    const uint16_t w = Width();
    const uint16_t h = Height();

    if (wipe_before_draw)
        super::unconditionalDraw();
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

void window_temp_graph_t::unconditionalDraw() {
    const bool redraw_graph = graph_invalid;
    if (!redraw_graph && IsInvalid()) {
        draw_axes(true, false);
        Validate();
        ;
        return;
    }

    marlin_vars_t *vars = marlin_vars();

    y_bed_c[0] = (uint8_t)(179 - (vars->temp_bed * 0.5F));
    y_nozzle_c[0] = (uint8_t)(179 - (vars->temp_nozzle * 0.5F));
    y_nozzle_t[0] = (uint8_t)(179 - (vars->target_nozzle * 0.5F));
    y_bed_t[0] = (uint8_t)(179 - (vars->target_bed * 0.5F));

    if (redraw_graph) {
        draw_axes(false, false); //draw now not overwrite real temperatures

        uint8_t i;
        uint8_t ynt = y_nozzle_t[0];
        uint8_t ybt = y_bed_t[0];
        uint8_t ync = y_nozzle_c[0];
        uint8_t ybc = y_bed_c[0];

        const uint16_t x = Left();
        const uint16_t y = Top();

        for (i = 0; i < 178; i++) {
            redraw_point(x + i + 1, y, &y_nozzle_t[i], GetBackColor(), color_extruder_t);
            y_nozzle_t[i] = y_nozzle_t[i + 1];

            redraw_point(x + i + 1, y, &y_bed_t[i], GetBackColor(), color_bed_t);
            y_bed_t[i] = y_bed_t[i + 1];

            redraw_point(x + i + 1, y, &y_nozzle_c[i], GetBackColor(), color_extruder_c);
            y_nozzle_c[i] = y_nozzle_c[i + 1];

            redraw_point(x + i + 1, y, &y_bed_c[i], GetBackColor(), color_bed_c);
            y_bed_c[i] = y_bed_c[i + 1];
        }

        //FIXME leaves trace in the graph but bed temp. does not do it
        redraw_last_point(x + i + 1, y + y_nozzle_t[i], y + ynt, GetBackColor(), color_extruder_t);
        y_nozzle_t[i] = ynt;

        redraw_last_point(x + i + 1, y + y_bed_t[i], y + ybt, GetBackColor(), color_bed_t);
        y_bed_t[i] = ybt;

        redraw_last_point(x + i + 1, y + y_nozzle_c[i], y + ync, GetBackColor(), color_extruder_c);
        y_nozzle_c[i] = ync;

        redraw_last_point(x + i + 1, y + y_bed_c[i], y + ybc, GetBackColor(), color_bed_c);
        y_bed_c[i] = ybc;

        draw_axes(false, true); //hides 0 values
        graph_invalid = false;
    }
}
