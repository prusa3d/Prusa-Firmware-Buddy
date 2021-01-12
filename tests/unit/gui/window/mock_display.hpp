/**
 * @file mock_display.hpp
 * @author Radek Vana
 * @brief class for display emulation
 *  all methods are static, because I need to pass them ad function pointers
 * @date 2021-01-11
 */

//mock_display.hpp
#pragma once
#include "Rect16.h"
#include "guitypes.hpp"
#include <inttypes.h>
#include <array>

class MockDisplay {
    constexpr static uint16_t cols = 240;
    constexpr static uint16_t rows = 320;
    constexpr static uint16_t buff_rows = 16;

public:
    constexpr static uint16_t Cols() { return cols; }
    constexpr static uint16_t Rows() { return rows; }
    constexpr static uint16_t BuffRows() { return buff_rows; }
    static void init();
    static void done() {}
    static void clear(color_t clr);
    static void set_pixel(point_ui16_t pt, color_t clr);
    static uint8_t *get_block(point_ui16_t start, point_ui16_t end) { return nullptr; }
    static void draw_line(point_ui16_t pt0, point_ui16_t pt1, color_t clr) {}
    static void draw_rect(Rect16 rc, color_t clr) {}
    static void fill_rect(Rect16 rc, color_t clr) {}
    static bool draw_charUnicode(point_ui16_t pt, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg) {}
    static void draw_icon(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop) {}
    static void draw_png(point_ui16_t pt, FILE *pf) {}

    void ClrBufferLine(int line);
    static uint8_t *getBuff() { return (uint8_t *)buffer; }

private:
    //singleton
    static MockDisplay &Instance(); //methods are static, can be private
    MockDisplay() = default;
    MockDisplay(MockDisplay &) = delete;

    std::array<std::array<color_t, cols>, rows> pixels;
    static uint32_t buffer[cols * buff_rows];
};
