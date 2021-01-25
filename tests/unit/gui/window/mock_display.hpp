/**
 * @file mock_display.hpp
 * @author Radek Vana
 * @brief class for display emulation
 *  all methods are static, because I need to pass them by function pointers
 * @date 2021-01-11
 */

//mock_display.hpp
#pragma once
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
    void clear(color_t clr);
    uint32_t GetpixelNativeColor(uint16_t point_x, uint16_t point_y);
    void SetpixelNativeColor(uint16_t point_x, uint16_t point_y, uint32_t clr);
    uint8_t *GetBlock(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) const;
    void FillRectNativeColor(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t nativeclr);

    uint8_t *getBuff() { return (uint8_t *)buffer; }
    void drawCharFromBuff(point_ui16_t pt, uint16_t w, uint16_t h);

    static MockDisplay &Instance();

private:
    //singleton
    MockDisplay() = default;
    MockDisplay(MockDisplay &) = delete;

    std::array<std::array<color_t, cols>, rows> pixels;
    uint32_t buffer[cols * buff_rows];
};
