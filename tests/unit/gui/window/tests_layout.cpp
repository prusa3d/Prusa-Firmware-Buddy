#include "catch2/catch.hpp"

#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "cmsis_os.h" //HAL_GetTick
#include "mock_windows.hpp"
#include "mock_display.hpp"
#include "GuiDefaults.hpp"
#include "window_text.hpp"
#include <memory>

//8 bit resolution 1px per row .. 1 byte per row
uint8_t font_dot_data[] = {
    0x00, // '0' 8 bit resolution
    0xff  // '1' 8 bit resolution
};

// 1 px font
font_t font_dot = { 1, 1, 1, 0, (uint16_t *)font_dot_data, '0', '1' };

//4 bit resolution 2 px per row .. 1 byte per row
uint8_t font_2dot_data[] = {
    // '0' empty square 2x2
    0x00, // '0' 2px 4 bit resolution line 0
    0x00, // '0' 2px 4 bit resolution line 1

    // '1' full square 2x2
    0xff, // '1' 2px 4 bit resolution line 0
    0xff  // '1' 2px 4 bit resolution line 1
};

//2x2 px font
font_t font_2dot = { 2, 2, 1, 0, (uint16_t *)font_2dot_data, '0', '1' };

font_t *GuiDefaults::Font = &font_dot;
font_t *GuiDefaults::FontBig = &font_dot;
font_t *GuiDefaults::FontMenuItems = &font_dot;
font_t *GuiDefaults::FontMenuSpecial = &font_dot;

//to be binded - static for easier debug
static TMockDisplay<240, 320, 16> MockDispBasic;
static TMockDisplay<5, 5, 256> MockDisp5x5;
static TMockDisplay<8, 4, 256> MockDisp8x4;
static TMockDisplay<8, 8, 256> MockDisp8x8;

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

static void Clear(color_t clr) {
    MockDisplay::Instance().clear(uint32_t(clr));
}

static void TestRectColor(Rect16 rect, color_t color) {
    for (int16_t X = rect.Left(); X < rect.Width(); ++X) {
        for (int16_t Y = rect.Top(); Y < rect.Height(); ++Y) {
            REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == uint32_t(color));
        }
    }
};

//Minuend: The number that is to be subtracted from.
//Subtrahend: The number that is to be subtracted.
static void TestRectDiffColor(Rect16 Minuend, Rect16 Subtrahend, color_t MinuendColor, color_t DiffColor) {
    for (int16_t X = Minuend.Left(); X < Minuend.Width(); ++X) {
        for (int16_t Y = Minuend.Top(); Y < Minuend.Height(); ++Y) {
            if (Subtrahend.Contain(point_i16_t({ X, Y }))) {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == uint32_t(DiffColor));
            } else {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == uint32_t(MinuendColor));
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
    Clear(color_disp);
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
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == uint32_t(cl_true));
            } else {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == uint32_t(cl_false));
            }
        }
    }
};

TEST_CASE("Window layout tests", "[window]") {
    MockDisplay::Bind(MockDispBasic);
    Clear(color_t::Black);
    TestRectColor(DispRect(), color_t::Black);

    SECTION("RECT") {
        TestDispRectDraw(Rect16(0, 0, 0, 0), color_t::Blue, color_t::White);

        TestDispRectDraw(DispRect(), color_t::Blue, color_t::White);

        TestDispRectDraw({ 1, 0, uint16_t(int(MockDisplay::Cols()) - 1), MockDisplay::Rows() }, color_t::Red, color_t::Black);

        TestDispRectDraw(Rect16(10, 20, 1, 2), color_t::Blue, color_t::White);

        TestDispRectDraw(Rect16(0, 0, 1, 2), color_t::Red, color_t::Blue);

        TestDispRectDraw(Rect16(10, 20, 1, 2), color_t::White, color_t::Black);

        TestDispRectDraw(Rect16(10, 20, 1, 2), color_t::Black, color_t::Blue);
    }

    SECTION("Singlechar Text ") {
        MockDisplay::Bind(MockDisp5x5);
        Clear(color_t::Black);
        TestRectColor(DispRect(), color_t::Black);

        //default padding
        window_text_t txt(nullptr,
            Rect16(0, 0, 1 + GuiDefaults::Padding.left + GuiDefaults::Padding.right, 1 + GuiDefaults::Padding.top + GuiDefaults::Padding.bottom),
            is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1")));
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(GuiDefaults::Padding.left, GuiDefaults::Padding.top, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_LEFT_TOP
        Clear(color_t::Black);
        txt.SetPadding({ 0, 0, 0, 0 });
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_CENTER
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_LEFT_CENTER
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::LeftCenter());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_LEFT_BOTTOM
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::LeftBottom());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_RIGHT_TOP
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::RightTop());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_RIGHT_CENTER
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::RightCenter());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_RIGHT_BOTTOM
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::RightBottom());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_CENTER_TOP
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::CenterTop());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        //no padding, ALIGN_CENTER_BOTTOM
        Clear(color_t::Black);
        txt.SetAlignment(Align_t::CenterBottom());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Singleline Text ") {
        MockDisplay::Bind(MockDisp8x4);
        Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), color_t::Red);
        window_text_t txt(nullptr, DispRect(), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1101")));
        txt.SetPadding({ 0, 0, 0, 0 });

        txt.SetAlignment(Align_t::LeftTop());
        txt.Draw();
        std::array<std::array<bool, 8>, 4> mask = { { { 1, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 1, 1, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Multiline Text") {
        MockDisplay::Bind(MockDisp5x5);
        Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), color_t::Red);
        window_text_t txt(nullptr, DispRect(), is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1\n0\n1")));
        txt.SetPadding({ 0, 0, 0, 0 });

        txt.SetAlignment(Align_t::LeftTop());
        txt.Draw();
        std::array<std::array<bool, 5>, 5> mask = { { { 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        txt.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)("111\n101\n10001")));
        Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 1, 1, 1, 0 }, { 0, 1, 0, 1, 0 }, { 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Multiline Text, font 2x2") {
        MockDisplay::Bind(MockDisp8x8);
        Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), color_t::Red);
        window_text_t txt(nullptr, DispRect(), is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1\n0\n1")));
        txt.font = &font_2dot;
        txt.SetPadding({ 0, 0, 0, 0 });

        txt.SetAlignment(Align_t::LeftTop());
        txt.Draw();
        std::array<std::array<bool, 8>, 8> mask = { { { 1, 1, 0, 0, 0, 0, 0, 0 },
            { 1, 1, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 },
            { 1, 1, 0, 0, 0, 0, 0, 0 },
            { 1, 1, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        /* Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        txt.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)("111\n101\n10001")));
        Clear(color_t::Red); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 1, 1, 1, 0 }, { 0, 1, 0, 1, 0 }, { 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);*/
    }
};
