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
    Instance().clear(COLOR_WHITE);
}

void MockDisplay::clear(color_t clr) {
    std::array<color_t, cols> row;
    row.fill(clr);
    pixels.fill(row);
}

uint32_t MockDisplay::GetpixelNativeColor(uint16_t point_x, uint16_t point_y) {
    return pixels[point_x][point_y];
}

void MockDisplay::SetpixelNativeColor(uint16_t point_x, uint16_t point_y, uint32_t clr) {
    pixels[point_x][point_y] = clr;
}

uint8_t *MockDisplay::GetBlock(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) const {
    return nullptr;
}
void MockDisplay::FillRectNativeColor(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t nativeclr) {}
void MockDisplay::ClrBufferLine(int line) {}

void MockDisplay::drawCharFromBuff(point_ui16_t pt, uint16_t w, uint16_t h) {}
