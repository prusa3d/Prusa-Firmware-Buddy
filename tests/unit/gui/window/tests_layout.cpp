#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "ScreenHandler.hpp"
#include "gui_time.hpp"
#include "mock_windows.hpp"
#include "mock_display.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include "window_text.hpp"
#include "knob_event.hpp"
#include <memory>
#include "display_helper.h"
#include <font_data/font_data.hpp>
#include <str_utils.hpp>

// The font data is opaque, so the tests do not link the font library and lay a font out
// themselves instead: two characters, ' ' first and '!' second.
namespace font_data {

struct FontData {
    const uint8_t *bitmap;
};

const uint8_t *Font::character_bitmap(uint32_t character) const {
    const uint32_t bytes_per_character = (w * h + 1) >> 1;
    return data->bitmap + (character == '!' ? bytes_per_character : 0);
}

bool Font::contains(uint32_t character) const {
    return character == ' ' || character == '!';
}

} // namespace font_data

// 8 bit resolution 1px per row .. 1 byte per row
uint8_t font_dot_data[] = {
    0x00, // ' ' 8 bit resolution
    0xff // '!' 8 bit resolution
};

// 1 px font
const font_data::FontData font_dot_font_data { font_dot_data };
font_t font_dot = { 1, 1, &font_dot_font_data };

// 4 bit resolution 2 px per row .. 1 byte per row
uint8_t font_2dot_data[] = {
    // ' ' empty square 2x2
    0x00, // ' ' 2px 4 bit resolution line 0
    0x00, // ' ' 2px 4 bit resolution line 1

    // '!' full square 2x2
    0xff, // '!' 2px 4 bit resolution line 0
    0xff // '!' 2px 4 bit resolution line 1
};

// 2x2 px font
const font_data::FontData font_2dot_font_data { font_2dot_data };
font_t font_2dot = { 2, 2, &font_2dot_font_data };

// to be binded - static for easier debug
static TMockDisplay<240, 320, 16> MockDispBasic;
static TMockDisplay<5, 5, 256> MockDisp5x5;
static TMockDisplay<8, 4, 256> MockDisp8x4;
static TMockDisplay<8, 8, 256> MockDisp8x8;

// stubbed header does not have C linkage .. to be simpler
static uint32_t hal_tick = 0;
uint32_t gui::GetTick() { return hal_tick; }
void gui::TickLoop() {}

point_ui16_t icon_meas(const uint8_t *pi) {
    point_ui16_t wh = { 0, 0 };
    return wh;
}

static bool g_use_font_2x2 = false;

const font_t *resource_font(Font) {
    return g_use_font_2x2 ? &font_2dot : &font_dot;
}

static Rect16 DispRect() { return Rect16(0, 0, MockDisplay::Cols(), MockDisplay::Rows()); }

static void TestRectColor(Rect16 rect, Color color) {
    for (int16_t X = rect.Left(); X < rect.Width(); ++X) {
        for (int16_t Y = rect.Top(); Y < rect.Height(); ++Y) {
            REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == color.raw);
        }
    }
};

// Minuend: The number that is to be subtracted from.
// Subtrahend: The number that is to be subtracted.
static void TestRectDiffColor(Rect16 Minuend, Rect16 Subtrahend, Color MinuendColor, Color DiffColor) {
    for (int16_t X = Minuend.Left(); X < Minuend.Width(); ++X) {
        for (int16_t Y = Minuend.Top(); Y < Minuend.Height(); ++Y) {
            if (Subtrahend.Contain(point_i16_t({ X, Y }))) {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == DiffColor.raw);
            } else {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == MinuendColor.raw);
            }
        }
    }
};

static void TestRectDraw(Rect16 rect, Color color) {
    BasicWindow win(nullptr, rect);
    win.SetBackColor(color);
    win.Draw();
    TestRectColor(rect, color);
};

static void TestDispRectDraw(Rect16 rect, Color color_win, Color color_disp) {
    MockDisplay::Instance().clear(color_disp);
    TestRectColor(DispRect(), color_disp);
    TestRectDraw(rect, color_win);
    TestRectDiffColor(DispRect(), rect, color_disp, color_win);
};

template <size_t COLS, size_t ROWS>
static void TestPixelMask(std::array<std::array<bool, COLS>, ROWS> mask, Color cl_false, Color cl_true) {
    REQUIRE(MockDisplay::Cols() == COLS);
    REQUIRE(MockDisplay::Rows() == ROWS);
    for (uint16_t X = 0; X < COLS; ++X) {
        for (uint16_t Y = 0; Y < ROWS; ++Y) {
            if (mask[Y][X]) {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == cl_true.raw);
            } else {
                REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == cl_false.raw);
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

    SECTION("Singlechar Text ") {
        MockDisplay::Bind(MockDisp5x5);
        MockDisplay::Instance().clear(COLOR_BLACK);
        TestRectColor(DispRect(), COLOR_BLACK);

        // default padding
        window_text_t txt(nullptr,
            Rect16(0, 0, 1 + GuiDefaults::Padding.left + GuiDefaults::Padding.right, 1 + GuiDefaults::Padding.top + GuiDefaults::Padding.bottom),
            is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH("!"));
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(GuiDefaults::Padding.left, GuiDefaults::Padding.top, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_LEFT_TOP
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetPadding({ 0, 0, 0, 0 });
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_CENTER
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_LEFT_CENTER
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::LeftCenter());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_LEFT_BOTTOM
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::LeftBottom());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(0, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_RIGHT_TOP
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::RightTop());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_RIGHT_CENTER
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::RightCenter());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 2, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_RIGHT_BOTTOM
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::RightBottom());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(4, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_CENTER_TOP
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::CenterTop());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 0, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);

        // no padding, ALIGN_CENTER_BOTTOM
        MockDisplay::Instance().clear(COLOR_BLACK);
        txt.SetAlignment(Align_t::CenterBottom());
        txt.Draw();
        TestRectDiffColor(DispRect(), Rect16(2, 4, 1, 1), GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Singleline Text ") {
        MockDisplay::Bind(MockDisp8x4);
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), COLOR_RED);
        window_text_t txt(nullptr, DispRect(), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH("!! !"));
        txt.SetPadding({ 0, 0, 0, 0 });

        txt.SetAlignment(Align_t::LeftTop());
        txt.Draw();
        std::array<std::array<bool, 8>, 4> mask = { { { 1, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 1, 1, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Multiline Text: Alignments") {
        MockDisplay::Bind(MockDisp5x5);
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), COLOR_RED);
        window_text_t txt(nullptr, DispRect(), is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH("!\n \n!"));
        txt.SetPadding({ 0, 0, 0, 0 });

        txt.SetAlignment(Align_t::LeftTop());
        txt.Draw();
        std::array<std::array<bool, 5>, 5> mask = { { { 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        txt.SetText(string_view_utf8::MakeCPUFLASH("!!!\n! !\n!   !"));
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 1, 1, 1, 0 }, { 0, 1, 0, 1, 0 }, { 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Multiline Text: font 2x2") {
        g_use_font_2x2 = true;
        MockDisplay::Bind(MockDisp8x8);
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), COLOR_RED);
        window_text_t txt(nullptr, DispRect(), is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH("!\n \n!"));
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
        g_use_font_2x2 = false;

        /* MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        txt.SetText(string_view_utf8::MakeCPUFLASH("111\n101\n10001"));
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 1, 1, 1, 0 }, { 0, 1, 0, 1, 0 }, { 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);*/
    }
}

TEST_CASE("RectTextLayout: Basics", "[layout]") {

    SECTION("Basic singleline") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1111111111"));
        auto layout = RectTextLayout(reader, 10, 1, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 1);
        REQUIRE(layout.get_width_in_chars() == 10);
        REQUIRE(layout.get_line_characters(0) == 10);
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Basic short singleline") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("11111"));
        auto layout = RectTextLayout(reader, 10, 1, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 1);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 5);
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Basic singleline single char") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1"));
        auto layout = RectTextLayout(reader, 1, 1, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 1);
        REQUIRE(layout.get_width_in_chars() == 1);
        REQUIRE(layout.get_line_characters(0) == 1);
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Empty str") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH(""));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 0);
        REQUIRE(layout.get_width_in_chars() == 0);
        REQUIRE(layout.get_line_characters(0) == 0);
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Empty space will always overflow") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH(""));
        auto layout = RectTextLayout(reader, 0, 0, is_multiline::no);
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Not enough height") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1"));
        auto layout = RectTextLayout(reader, 0, 1, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 0);
        REQUIRE(layout.get_width_in_chars() == 0);
        REQUIRE(layout.get_line_characters(0) == 0);
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Not enough width") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1"));
        auto layout = RectTextLayout(reader, 1, 0, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 0);
        REQUIRE(layout.get_width_in_chars() == 0);
        REQUIRE(layout.get_line_characters(0) == 0);
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Basic singleline overflow") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1111111111"));
        auto layout = RectTextLayout(reader, 5, 1, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 1);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 5);
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Basic multiline") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("11111\n1111\n111\n11\n1"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 5);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 5);
        REQUIRE(layout.get_line_characters(1) == 4);
        REQUIRE(layout.get_line_characters(2) == 3);
        REQUIRE(layout.get_line_characters(3) == 2);
        REQUIRE(layout.get_line_characters(4) == 1);
    }

    SECTION("Longest line test") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111\n0\n11111\n000\n1"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 5);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 3);
        REQUIRE(layout.get_line_characters(1) == 1);
        REQUIRE(layout.get_line_characters(2) == 5);
        REQUIRE(layout.get_line_characters(3) == 3);
        REQUIRE(layout.get_line_characters(4) == 1);
    }

    SECTION("Newline test") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1\n0\n1\n1\n1"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 5);
        REQUIRE(layout.get_width_in_chars() == 1);
        REQUIRE(layout.get_line_characters(0) == 1);
        REQUIRE(layout.get_line_characters(1) == 1);
        REQUIRE(layout.get_line_characters(2) == 1);
        REQUIRE(layout.get_line_characters(3) == 1);
        REQUIRE(layout.get_line_characters(4) == 1);
    }
}

TEST_CASE("RectTextLayout: input misalignment", "[layout]") {

    SECTION("Newline is last character") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("11111\n"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 2); // Correctly its 2, because it is a newline text and new line character adds new line
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 5);
        REQUIRE(layout.get_line_characters(1) == 0);
        REQUIRE(layout.get_line_characters(2) == 0);
    }

    SECTION("Multiline set to NO") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1\n11\n111"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::no); // Multiline set to NO
        REQUIRE(layout.get_height_in_chars() == 1);
        REQUIRE(layout.get_width_in_chars() == 1);
        REQUIRE(layout.get_line_characters(0) == 1);
        REQUIRE(layout.get_line_characters(1) == 0);
        REQUIRE(layout.get_line_characters(2) == 0);
        REQUIRE(layout.get_line_characters(3) == 0);
    }

    SECTION("First line is too long") {
        // It should return print out cropped word and the rest on the next row
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111111\n11\n111"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 4);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 5);
        REQUIRE(layout.get_line_characters(1) == 1);
        REQUIRE(layout.get_line_characters(2) == 2);
        REQUIRE(layout.get_line_characters(3) == 3);
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Some line is too long") {
        // It should return print out cropped word and the rest on the next row
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111\n1 11\n1111111\n11"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 5);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 3);
        REQUIRE(layout.get_line_characters(1) == 4);
        REQUIRE(layout.get_line_characters(2) == 5);
        REQUIRE(layout.get_line_characters(3) == 2);
        REQUIRE(layout.get_line_characters(4) == 2);
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Line too long with space at the end") {
        // It should return print out cropped word and the rest on the next row
        // In this case, space is handled like a newline character (skipped in the next row)
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111\n11111 1111\n11"));
        auto layout = RectTextLayout(reader, 5, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 4);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 3);
        REQUIRE(layout.get_line_characters(1) == 5);
        REQUIRE(layout.get_line_characters(2) == 4);
        REQUIRE(layout.get_line_characters(3) == 2);
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Not enough height") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111\n1 11\n1 11\n11"));
        auto layout = RectTextLayout(reader, 5, 3, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 3);
        REQUIRE(layout.get_width_in_chars() == 4);
        REQUIRE(layout.get_line_characters(0) == 3);
        REQUIRE(layout.get_line_characters(1) == 4);
        REQUIRE(layout.get_line_characters(2) == 4);
        REQUIRE(layout.get_line_characters(3) == 0);
        REQUIRE(layout.has_text_overflown() == true);
    }
}

TEST_CASE("RextTextLayout: Word wrapping japanese", "[layout]") {

    SECTION("Japanese comma & dot: text wrap #1") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("ロードセルキャリ。プリンタロードセルキャリ、ードセルキャリロードセルキャリ"));
        auto layout = RectTextLayout(reader, 15, 3, is_multiline::yes);
        CHECK(layout.get_height_in_chars() == 3);
        CHECK(layout.get_width_in_chars() == 15);
        CHECK(layout.get_line_characters(0) == 9);
        CHECK(layout.get_line_characters(1) == 13);
        CHECK(layout.get_line_characters(2) == 15);
        CHECK(layout.has_text_overflown() == false);
    }

    SECTION("Japanese comma & dot: text wrap #2") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("ロードセルキャリリ リンタロードセルキャリ、ードセルキャリローー"));
        auto layout = RectTextLayout(reader, 10, 4, is_multiline::yes);
        CHECK(layout.get_height_in_chars() == 4);
        CHECK(layout.get_width_in_chars() == 10);
        CHECK(layout.get_line_characters(0) == 9);
        CHECK(layout.get_line_characters(1) == 10);
        CHECK(layout.get_line_characters(2) == 2);
        CHECK(layout.get_line_characters(3) == 10);
        CHECK(layout.has_text_overflown() == true);
    }
}

TEST_CASE("RextTextLayout: Word wrapping", "[layout]") {

    SECTION("Singleline texts do not wrap") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1 1 111111"));
        auto layout = RectTextLayout(reader, 6, 1, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 1);
        REQUIRE(layout.get_width_in_chars() == 6);
        REQUIRE(layout.get_line_characters(0) == 6); // '1 1 11'
        REQUIRE(layout.get_line_characters(1) == 0);
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Complex wrap situation #1") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111 00\n1\n11 000\n1111"));
        auto layout = RectTextLayout(reader, 5, 6, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 6);
        REQUIRE(layout.get_width_in_chars() == 4);
        REQUIRE(layout.get_line_characters(0) == 3); // 111
        REQUIRE(layout.get_line_characters(1) == 2); // 00
        REQUIRE(layout.get_line_characters(2) == 1); // 1
        REQUIRE(layout.get_line_characters(3) == 2); // 11
        REQUIRE(layout.get_line_characters(4) == 3); // 000
        REQUIRE(layout.get_line_characters(5) == 4); // 1111
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Complex wrap situation #2") {
        // Multiwrap of a single line
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1111 00 111 1100 1 111"));
        auto layout = RectTextLayout(reader, 5, 6, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 5);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 4); // 1111
        REQUIRE(layout.get_line_characters(1) == 2); // 00
        REQUIRE(layout.get_line_characters(2) == 3); // 111
        REQUIRE(layout.get_line_characters(3) == 4); // 1100
        REQUIRE(layout.get_line_characters(4) == 5); // 1 111
        REQUIRE(layout.get_line_characters(5) == 0);
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Complex wrap situation #3") {
        // Only one whitespace is emitted on the end of the line (other whitespaces are intended)
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111 222 000 111  000 111 \n 0000"));
        auto layout = RectTextLayout(reader, 10, 4, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 4);
        REQUIRE(layout.get_width_in_chars() == 8);
        REQUIRE(layout.get_line_characters(0) == 7); // '111 222'
        REQUIRE(layout.get_line_characters(1) == 8); // '000 111 ' (space at the end)
        REQUIRE(layout.get_line_characters(2) == 8); // '000 111 ' (space at the end)
        REQUIRE(layout.get_line_characters(3) == 5); // ' 0000' (space at the beginning)
        REQUIRE(layout.has_text_overflown() == false);
    }

    SECTION("Complex wrap situation #4") {
        // Whitespace edge cases
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1111 \n11111 \n      "));
        auto layout = RectTextLayout(reader, 5, 4, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 4);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 5); // '1111 ' (space at the end)
        REQUIRE(layout.get_line_characters(1) == 5); // '11111'
        REQUIRE(layout.get_line_characters(2) == 0); // '' (empty)
        REQUIRE(layout.get_line_characters(3) == 5); // '     ' (5x space)
        REQUIRE(layout.has_text_overflown() == true); // ther is no space for the last space
    }

    SECTION("Complex wrap situation #5") {
        // Word is too long for a line
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1 1 111111"));
        auto layout = RectTextLayout(reader, 5, 3, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 3);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 3); // '1 1'
        REQUIRE(layout.get_line_characters(1) == 5); // '11111'
        REQUIRE(layout.get_line_characters(2) == 1); // '1'
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Limited space 5x1") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("111111111"));
        auto layout = RectTextLayout(reader, 5, 1, is_multiline::no);
        REQUIRE(layout.get_height_in_chars() == 1);
        REQUIRE(layout.get_width_in_chars() == 5);
        REQUIRE(layout.get_line_characters(0) == 5);
        REQUIRE(layout.get_line_characters(1) == 0);
        REQUIRE(layout.has_text_overflown() == true);
    }

    SECTION("Limited space 1x5") {
        StringReaderUtf8 reader(string_view_utf8::MakeCPUFLASH("1 1 1 1 1"));
        auto layout = RectTextLayout(reader, 1, 5, is_multiline::yes);
        REQUIRE(layout.get_height_in_chars() == 5);
        REQUIRE(layout.get_width_in_chars() == 1);
        REQUIRE(layout.get_line_characters(0) == 1);
        REQUIRE(layout.get_line_characters(1) == 1);
        REQUIRE(layout.get_line_characters(2) == 1);
        REQUIRE(layout.get_line_characters(3) == 1);
        REQUIRE(layout.get_line_characters(4) == 1);
        REQUIRE(layout.has_text_overflown() == false);
    }
}

// Frame is different than screen (cannot host subwindows)
// also check if frame is notified about visibility changes
TEST_CASE("Visibility notifycation test", "[window]") {
    MockDisplay::Bind(MockDispBasic);
    MockDisplay::Instance().clear(COLOR_BLACK);
    MockFrame_VisibilityNotifycations frame;

    window_t &win = frame.win;
    frame.Draw(); // clears invalidation rect

    // no need to call HideBehindDialog, because frame cannot host dialogs

    REQUIRE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());
    REQUIRE(frame.ChangedCounter == 0);
    REQUIRE(frame.GetInvRect().IsEmpty());

    win.Hide();

    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE_FALSE(win.IsCapturable());
    REQUIRE(frame.ChangedCounter == 1);
    REQUIRE(frame.GetInvRect() == win.GetRect());
    frame.ChangedCounter = 0;
    frame.Draw(); // clears invalidation rect

    // unregistration must invalidate rect
    // mock does not increase counter
    frame.UnregisterSubWin(win);
    REQUIRE(frame.GetInvRect() == win.GetRect());
    frame.Draw(); // clears invalidation rect

    // registration cannot invalidate rect
    // mock does not increase counter
    frame.RegisterSubWin(win);
    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE_FALSE(win.IsCapturable());
    REQUIRE(frame.GetInvRect().IsEmpty());
    frame.Draw(); // clears invalidation rect

    win.Show();

    REQUIRE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());
    REQUIRE(frame.ChangedCounter == 1);
    frame.ChangedCounter = 0;
    frame.Draw(); // clears invalidation rect

    // unregistration must invalidate rect
    // mock does not increase counter
    frame.UnregisterSubWin(win);
    REQUIRE(frame.GetInvRect() == win.GetRect());
    frame.Draw(); // clears invalidation rect
}

TEST_CASE("Visibility notifycation test, hidden win registration", "[window]") {
    MockDisplay::Bind(MockDispBasic);
    MockDisplay::Instance().clear(COLOR_BLACK);
    Rect16 rc = GENERATE(GuiDefaults::RectScreen, Rect16(20, 20, 20, 20));
    MockScreen screen;
    screen.Draw(); // clear invalidation rect

    BasicWindow win(nullptr, rc);
    win.Hide();

    screen.RegisterSubWin(win); // register hidden win
    REQUIRE(screen.GetInvRect().IsEmpty()); // hidden win should not set invalidation rect
}

TEST_CASE("Capturable test window in screen", "[window]") {
    MockDisplay::Bind(MockDispBasic);
    MockDisplay::Instance().clear(COLOR_BLACK);
    MockScreen screen;
    Screens::Access()->Set(&screen); // instead of screen registration
    screen.Draw(); // clears invalidation rect

    // just test one of windows, does not matter which one
    window_t &win = screen.w0;

    REQUIRE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());
    REQUIRE(screen.GetInvRect().IsEmpty());
    screen.Draw();

    win.HideBehindDialog(); // cannot not change rect

    REQUIRE_FALSE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE(win.IsHiddenBehindDialog());
    REQUIRE_FALSE(win.IsCapturable());
    REQUIRE(screen.GetInvRect().IsEmpty()); // hide behind dialog only invalidates dialog
    screen.Draw();

    win.Hide();

    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog()); // screen cleared this flag, because of Hide notifycation
    REQUIRE_FALSE(win.IsCapturable());
    REQUIRE(screen.GetInvRect() == win.GetRect());
    screen.Draw();

    win.SetEnforceCapture();

    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());
}

TEST_CASE("Timed dialog tests", "[window]") {
    MockDisplay::Bind(MockDispBasic);
    MockDisplay::Instance().clear(COLOR_BLACK);
    MockScreen screen;
    Screens::Access()->Set(&screen); // instead of screen registration

    // initial screen check
    screen.BasicCheck();
    REQUIRE(screen.GetCapturedWindow() == &screen);

    SECTION("Screen timed dialog test") {
        static constexpr Rect16 RC2(20, 20, 20, 20);
        Rect16 rc = GENERATE(GuiDefaults::RectScreen, RC2);
        screen.Draw();
        screen.BasicCheck();
        REQUIRE(screen.GetCapturedWindow() == &screen);
        REQUIRE(screen.GetInvalidationRect().IsEmpty()); // cleared by draw

        MockDialogTimed dlg_timed(&screen, rc);
        REQUIRE(screen.GetInvRect() == rc);

        dlg_timed.Show(); // win is hidden by default
        REQUIRE(screen.GetInvRect() == rc); // unlike frame, screen invalidates on Show too, (multiple dialog priority, far shorter code)

        dlg_timed.Hide();
        REQUIRE(screen.GetInvRect() == rc);

        screen.Draw();
        REQUIRE(screen.GetInvalidationRect().IsEmpty()); // cleared by draw
    }

    hal_tick = 1000; // set opened on popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); // loop will initialize popup timeout
    hal_tick = 10000; // timeout popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); // loop event will unregister popup

    // at the end of all sections screen must be returned to its original state
    screen.BasicCheck();
    REQUIRE(screen.GetCapturedWindow() == &screen);
}
