#include "catch2/catch.hpp"

#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "gui_time.hpp" //gui::GetTick
#include "mock_windows.hpp"
#include "mock_display.hpp"
#include "GuiDefaults.hpp"
#include "window_text.hpp"
#include "knob_event.hpp"
#include <memory>

// 8 bit resolution 1px per row .. 1 byte per row
uint8_t font_dot_data[] = {
    0x00, // '0' 8 bit resolution
    0xff // '1' 8 bit resolution
};

// 1 px font
font_t font_dot = { 1, 1, 1, 0, (uint16_t *)font_dot_data, '0', '1' };

// 4 bit resolution 2 px per row .. 1 byte per row
uint8_t font_2dot_data[] = {
    // '0' empty square 2x2
    0x00, // '0' 2px 4 bit resolution line 0
    0x00, // '0' 2px 4 bit resolution line 1

    // '1' full square 2x2
    0xff, // '1' 2px 4 bit resolution line 0
    0xff // '1' 2px 4 bit resolution line 1
};

// 2x2 px font
font_t font_2dot = { 2, 2, 1, 0, (uint16_t *)font_2dot_data, '0', '1' };

font_t *GuiDefaults::Font = &font_dot;
font_t *GuiDefaults::FontBig = &font_dot;
font_t *GuiDefaults::FontMenuItems = &font_dot;
font_t *GuiDefaults::FontMenuSpecial = &font_dot;

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

static Rect16 DispRect() { return Rect16(0, 0, MockDisplay::Cols(), MockDisplay::Rows()); }

static void TestRectColor(Rect16 rect, color_t color) {
    for (int16_t X = rect.Left(); X < rect.Width(); ++X) {
        for (int16_t Y = rect.Top(); Y < rect.Height(); ++Y) {
            REQUIRE(MockDisplay::Instance().GetpixelNativeColor(X, Y) == color);
        }
    }
};

// Minuend: The number that is to be subtracted from.
// Subtrahend: The number that is to be subtracted.
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

    SECTION("Singlechar Text ") {
        MockDisplay::Bind(MockDisp5x5);
        MockDisplay::Instance().clear(COLOR_BLACK);
        TestRectColor(DispRect(), COLOR_BLACK);

        // default padding
        window_text_t txt(nullptr,
            Rect16(0, 0, 1 + GuiDefaults::Padding.left + GuiDefaults::Padding.right, 1 + GuiDefaults::Padding.top + GuiDefaults::Padding.bottom),
            is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1")));
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
        window_text_t txt(nullptr, DispRect(), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1101")));
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

    SECTION("Multiline Text") {
        MockDisplay::Bind(MockDisp5x5);
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), COLOR_RED);
        window_text_t txt(nullptr, DispRect(), is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1\n0\n1")));
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

        txt.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)("111\n101\n10001")));
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 1, 1, 1, 0 }, { 0, 1, 0, 1, 0 }, { 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);
    }

    SECTION("Multiline Text, font 2x2") {
        MockDisplay::Bind(MockDisp8x8);
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        TestRectColor(DispRect(), COLOR_RED);
        window_text_t txt(nullptr, DispRect(), is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)("1\n0\n1")));
        txt.set_font(&font_2dot);
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

        /* MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);

        txt.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)("111\n101\n10001")));
        MockDisplay::Instance().clear(COLOR_RED); // all display must be rewritten, no red pixel can remain
        txt.SetAlignment(Align_t::Center());
        txt.Draw();
        mask = { { { 0, 0, 0, 0, 0 }, { 0, 1, 1, 1, 0 }, { 0, 1, 0, 1, 0 }, { 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } } };
        TestPixelMask(mask, GuiDefaults::ColorBack, GuiDefaults::ColorText);*/
    }
};

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

    window_t win(nullptr, rc);
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
