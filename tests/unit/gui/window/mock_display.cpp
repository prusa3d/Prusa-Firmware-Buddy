/**
 * @file mock_display.cpp
 * @author Radek Vana
 * @date 2021-01-11
 */
#include "mock_display.hpp"

MockDisplay &MockDisplay::Instance() {
    static MockDisplay ret;
    return ret;
}

void MockDisplay::init() {
    clear(COLOR_WHITE);
}

void MockDisplay::clear(color_t clr) {
    std::array<color_t, cols> row;
    row.fill(clr);
    Instance().pixels.fill(row);
}

void MockDisplay::set_pixel(point_ui16_t pt, color_t clr) {
    Instance().pixels[pt.x][pt.y] = clr;
}
