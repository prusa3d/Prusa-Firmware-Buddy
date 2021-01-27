#include "catch2/catch.hpp"

#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "cmsis_os.h" //HAL_GetTick
#include "mock_windows.hpp"
#include "mock_display.hpp"
#include "GuiDefaults.hpp"
#include "window_text.hpp"
#include <memory>

uint8_t font_dot_data[] = {
    0x00,
    0xff // 0x80 ? or 0x01?
};

font_t font_dot = { 1, 1, 1, 0, (uint16_t *)font_dot_data, '0', '1' };

font_t *GuiDefaults::Font = &font_dot;
font_t *GuiDefaults::FontBig = &font_dot;
font_t *GuiDefaults::FontMenuItems = &font_dot;
font_t *GuiDefaults::FontMenuSpecial = &font_dot;

//to be binded - static for easier debug
static TMockDisplay<240, 320, 16> MockDispBasic;
static TMockDisplay<5, 5, 256> MockDisp5x5;

//stubbed header does not have C linkage .. to be simpler
static uint32_t hal_tick = 0;
uint32_t HAL_GetTick() { return hal_tick; }
const uint8_t *resource_ptr(uint16_t id) {
    return 0;
}
point_ui16_t icon_meas(const uint8_t *pi) {
    point_ui16_t wh = { 0, 0 };
    return wh;
}

static Rect16 DispRect() { return Rect16(0, 0, MockDisplay::Cols(), MockDisplay::Rows()); }

static void TestRectColor(Rect16 rect, color_t color) {
    for (int16_t X = rect.Left(); X < rect.Width(); ++X) {
        for (int16_t Y = rect.Top(); Y < rect.Height(); ++Y) {
            REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == color);
        }
    }
};

//Minuend: The number that is to be subtracted from.
//Subtrahend: The number that is to be subtracted.
static void TestRectDiffColor(Rect16 Minuend, Rect16 Subtrahend, color_t MinuendColor, color_t DiffColor) {
    for (int16_t X = Minuend.Left(); X < Minuend.Width(); ++X) {
        for (int16_t Y = Minuend.Top(); Y < Minuend.Height(); ++Y) {
            if (Subtrahend.Contain(point_i16_t({ X, Y }))) {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == DiffColor);
            } else {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == MinuendColor);
            }
        }
    }
};

static void TestRectDraw(Rect16 rect, color_t color) {
    window_t win(nullptr, rect);
    win.SetBackColor(color);
    win.Draw();
    TestRectColor(rect, color);
};

static void TestDispRectDraw(Rect16 rect, color_t color_win, color_t color_disp) {
    MockDisplay::Instance().clear(color_disp);
    TestRectColor(DispRect(), color_disp);
    TestRectDraw(rect, color_win);
    TestRectDiffColor(DispRect(), rect, color_disp, color_win);
};

template <size_t COLS, size_t ROWS>
static void TestPixelMask(std::array<std::array<bool, COLS>, ROWS> mask, color_t cl_false, color_t cl_true) {
    REQUIRE(MockDisplay::Cols() == COLS);
    REQUIRE(MockDisplay::Rows() == ROWS);
    for (uint16_t X = 0; X < COLS; ++X) {
        for (uint16_t Y = 0; Y < ROWS; ++Y) {
            if (mask[Y][X]) {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == cl_true);
            } else {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == cl_false);
            }
        }
    }
};

TEST_CASE("Window layout tests", "[window]") {
    MockDisplay::Bind(MockDispBasic);
    MockDisplay::Instance().clear(COLOR_BLACK);
    TestRectColor(DispRect(), COLOR_BLACK);

    SECTION("RECT") {
        TestDispRectDraw(Rect16(0, 0, 0, 0), COLOR_BLUE, COLOR_WHITE);

        TestDispRectDraw(DispRect(), COLOR_BLUE, COLOR_WHITE);

        TestDispRectDraw({ 1, 0, uint16_t(int(MockDisplay::Cols()) - 1), MockDisplay::Rows() }, COLOR_RED, COLOR_BLACK);

        TestDispRectDraw(Rect16(10, 20, 1, 2), COLOR_BLUE, COLOR_WHITE);

        TestDispRectDraw(Rect16(0, 0, 1, 2), COLOR_RED, COLOR_BLUE);

        TestDispRectDraw(Rect16(10, 20, 1, 2), COLOR_WHITE, COLOR_BLACK);

        TestDispRectDraw(Rect16(10, 20, 1, 2), COLOR_BLACK, COLOR_BLUE);
    }

    SECTION("Singleline Text") {
        MockDisplay::Bind(MockDisp5x5);
        MockDisplay::Instance().clear(COLOR_BLACK);
        TestRectColor(DispRect(), COLOR_BLACK);

        //default padding
        window_text_t txt(nullptr,
            Rect16(0, 0, 1 + GuiDefaults::Padding.left + GuiDefaults::Padding.right, 1 + GuiDefaults::Padding.top + GuiDefaults::Padding.bottom),
            is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1")));
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(GuiDefaults::Padding.left, GuiDefaults::Padding.top, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_LEFT_TOP
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetPadding({ 0, 0, 0, 0 });
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_CENTER
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_CENTER);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_LEFT_CENTER
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_LEFT_CENTER);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_LEFT_BOTTOM
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_LEFT_BOTTOM);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_RIGHT_TOP
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_RIGHT_TOP);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_RIGHT_CENTER
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_RIGHT_CENTER);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_RIGHT_BOTTOM
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_RIGHT_BOTTOM);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_CENTER_TOP
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_CENTER_TOP);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_CENTER_BOTTOM
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_CENTER_BOTTOM);
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Multiline Text") {
        MockDisplay::Bind(MockDisp5x5);
        MockDisplay::Instance().clear(COLOR_BLACK);
        TestRectColor(DispRect(), COLOR_BLACK);
        window_text_t txt(nullptr, DispRect(), is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1\n0\n1")));
        txt.SetPadding({ 0, 0, 0, 0 });

        txt.SetAlignment(ALIGN_LEFT_TOP);
        txt.Draw();
        std::array<std::array<bool, 5>, 5> mask_left_top = { { { 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask_left_top, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(ALIGN_CENTER);
        txt.Draw();
        std::array<std::array<bool, 5>, 5> mask_center = { { { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask_center, GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }
};
