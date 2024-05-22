/**
 * @file mock_display.hpp
 * @brief class for display emulation
 *  all methods are static, because I need to pass them by function pointers
 */

// mock_display.hpp
#pragma once
#include "guitypes.hpp"
#include <inttypes.h>
#include <memory>
#include <array>

// interface
class IMockDisplay {
public:
    virtual uint16_t Cols() = 0;
    virtual uint16_t Rows() = 0;
    virtual uint16_t BuffRows() = 0;
    virtual void clear(color_t clr) = 0;
    virtual uint32_t GetpixelNativeColor(uint16_t point_x, uint16_t point_y) = 0;
    virtual void SetpixelNativeColor(uint16_t point_x, uint16_t point_y, uint32_t clr) = 0;
    virtual uint8_t *GetBlock(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) const = 0;
    virtual void FillRectNativeColor(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t nativeclr) = 0;
    virtual uint8_t *getBuff() = 0;
    virtual void drawFromBuff(point_ui16_t pt, uint16_t w, uint16_t h) = 0;
    virtual ~IMockDisplay() = default;
};

// template for different display sizes
template <uint16_t COLS, uint16_t ROWS, uint16_t BUFF_ROWS>
class TMockDisplay : public IMockDisplay {
public:
    virtual uint16_t Cols() override { return COLS; }
    virtual uint16_t Rows() override { return ROWS; }
    virtual uint16_t BuffRows() override { return BUFF_ROWS; }
    virtual void clear(color_t clr) override {
        std::array<color_t, COLS> row;
        row.fill(clr);
        pixels.fill(row);
    }
    virtual uint32_t GetpixelNativeColor(uint16_t point_x, uint16_t point_y) override {
        return pixels[point_y][point_x];
    }
    virtual void SetpixelNativeColor(uint16_t point_x, uint16_t point_y, uint32_t clr) override {
        pixels[point_y][point_x] = clr;
    }
    virtual uint8_t *GetBlock(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) const { return nullptr; }
    virtual void FillRectNativeColor(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t nativeclr) override {
        for (size_t row = 0; row < rect_h; ++row) {
            for (size_t col = 0; col < rect_w; ++col) {
                SetpixelNativeColor(rect_x + col, rect_y + row, nativeclr);
            }
        }
    }

    virtual uint8_t *getBuff() override { return (uint8_t *)buffer; }
    virtual void drawFromBuff(point_ui16_t pt, uint16_t w, uint16_t h) override {
        size_t buff_pos = 0;
        for (uint16_t Y = pt.y; Y < (h + pt.y); ++Y) {
            for (uint16_t X = pt.x; X < (w + pt.x); ++X) {
                SetpixelNativeColor(X, Y, buffer[buff_pos]);
                ++buff_pos;
            }
        }
    }

private:
    std::array<std::array<color_t, COLS>, ROWS> pixels;
    uint32_t buffer[COLS * BUFF_ROWS];
};

class MockDisplay {
public:
    static uint16_t Cols();
    static uint16_t Rows();
    static uint16_t BuffRows();
    static void init();
    static void done() {}
    static void set_backlight(uint8_t bck) {}

    static bool is_reset_required() {
        return false;
    }

    static void Reset() {}

    static IMockDisplay &Instance();
    static void Bind(IMockDisplay &disp);

private:
    static IMockDisplay *instance;
};
