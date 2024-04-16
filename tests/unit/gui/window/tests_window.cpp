#include "catch2/catch.hpp"

#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "gui_time.hpp" //gui::GetTick
#include "mock_windows.hpp"
#include "knob_event.hpp"
#include <memory>

// stubbed header does not have C linkage .. to be simpler
static uint32_t hal_tick = 0;
uint32_t gui::GetTick() { return hal_tick; }
void gui::TickLoop() {}

TEST_CASE("Window registration tests", "[window]") {
    MockScreen screen;
    Screens::Access()->Set(&screen); // instead of screen registration

    // initial screen check
    screen.BasicCheck();
    REQUIRE(screen.GetCapturedWindow() == &screen);

    SECTION("popup with no rectangle") {
        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        screen.BasicCheck(1); // basic check must pass, because rect is empty
        // popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); // popup does not claim capture
        screen.CheckOrderAndVisibility(popup);
    }

    SECTION("popup hiding w0 - w4") {
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()), string_view_utf8::MakeNULLSTR());
        // popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); // popup does not claim capture
        screen.CheckOrderAndVisibility(popup);
    }

    SECTION("msgbox with no rectangle") {
        MockMsgBox msgbox(Rect16(0, 0, 0, 0));
        screen.BasicCheck(0, 1); // basic check must pass, because rect is empty
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("msgbox hiding w0 - w4") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("popup with no rectangle + msgbox hiding w0 - w4") {
        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        screen.BasicCheck(1); // basic check must pass, because rect is empty
        // popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); // popup does not claim capture
        screen.CheckOrderAndVisibility(popup);

        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(popup->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox, popup);
    }

    SECTION("msgbox hiding w0 - w4 + popup with no rectangle") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);

        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        // popup is singleton must get its pointer from screen
        window_t *popup = msgbox.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(popup->GetParent() == &screen);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // popup does not claim capture
        screen.CheckOrderAndVisibility(&msgbox, popup);
    }

    SECTION("popup inside msgbox hiding w0 - w4") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()), string_view_utf8::MakeNULLSTR());
        // popup cannot open so test is same as if only msgbox is opened
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("msgbox closing popup hiding w0 - w4") {
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()), string_view_utf8::MakeNULLSTR());
        // popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); // popup does not claim capture
        screen.CheckOrderAndVisibility(popup);

        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        // popup must autoclose so test is same as if only msgbox is opened
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("hidden dialog and popup") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        // popup must autoclose so test is same as if only msgbox is opened
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); // msgbox does claim capture
        msgbox.Hide();
        REQUIRE_FALSE(screen.GetCapturedWindow() == &msgbox); // msgbox does not claim capture, because it is not visible

        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()), string_view_utf8::MakeNULLSTR());
        // use GetfirstPopUp next window after last screen is MockMsgBox
        window_t *popup = screen.GetFirstPopUp();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
    }

    SECTION("live adj Z + M600") {
        // emulate by 2 nested msgboxes
        MockMsgBox msgbox0(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        REQUIRE(msgbox0.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox0); // msgbox0 does claim capture
        screen.CheckOrderAndVisibility(&msgbox0);

        {
            MockMsgBox msgbox1(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
            REQUIRE(msgbox0.GetParent() == &screen);
            REQUIRE(msgbox1.GetParent() == &screen);
            REQUIRE(screen.GetCapturedWindow() == &msgbox1); // msgbox1 does claim capture
            screen.CheckOrderAndVisibility(&msgbox0, &msgbox1);
        }

        // retest after first msgbox is closed
        REQUIRE(msgbox0.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox0); // msgbox0 must get capture back
        screen.CheckOrderAndVisibility(&msgbox0);
    }

    SECTION("Unregister 2nd messagebox before 1st") {
        auto msgbox0 = std::make_unique<MockMsgBox>(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        REQUIRE(msgbox0->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == msgbox0.get()); // msgbox0 does claim capture
        screen.CheckOrderAndVisibility(msgbox0.get());

        auto msgbox1 = std::make_unique<MockMsgBox>(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()));
        REQUIRE(msgbox0->GetParent() == &screen);
        REQUIRE(msgbox1->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == msgbox1.get()); // msgbox1 does claim capture
        screen.CheckOrderAndVisibility(msgbox0.get(), msgbox1.get());

        // destroy msgbox0 before msgbox1
        msgbox0.reset();

        // retest after second msgbox is closed
        REQUIRE(msgbox1->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == msgbox1.get()); // msgbox1 must remain captured
        screen.CheckOrderAndVisibility(msgbox1.get());
    }

    SECTION("normal window") {
        screen.CaptureNormalWindow(screen.w0);
        screen.BasicCheck();
        REQUIRE(screen.GetCapturedWindow() == &screen.w0);
        screen.ReleaseCaptureOfNormalWindow();
    }

    SECTION("popup hiding w0 - w4 + normal window") {
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.GetRect(), screen.w1.GetRect(), screen.w2.GetRect(), screen.w3.GetRect()), string_view_utf8::MakeNULLSTR());
        // popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); // popup does not claim capture
        screen.CheckOrderAndVisibility(popup);

        screen.CaptureNormalWindow(screen.w0);
        REQUIRE(screen.GetCapturedWindow() == &screen.w0);
        screen.ReleaseCaptureOfNormalWindow();
    }

    hal_tick = 1000; // set opened on popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); // loop will initialize popup timeout
    hal_tick = 10000; // timeout popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); // loop event will unregister popup

    // at the end of all sections screen must be returned to its original state
    screen.BasicCheck();
    REQUIRE(screen.GetCapturedWindow() == &screen);
}

TEST_CASE("Capturable test, all combinations", "[window]") {
    window_t win(nullptr, Rect16(20, 20, 10, 10));

    // default
    // 1 .. visible
    // 0 .. enforced capture
    // 0 .. hidden behind dialog
    REQUIRE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());

    win.Hide();
    // 0 .. visible
    // 0 .. enforced capture
    // 0 .. hidden behind dialog
    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE_FALSE(win.IsCapturable());

    win.SetEnforceCapture();
    // 0 .. visible
    // 1 .. enforced capture
    // 0 .. hidden behind dialog
    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());

    win.HideBehindDialog();
    // 0 .. visible
    // 1 .. enforced capture
    // 1 .. hidden behind dialog
    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE(win.HasEnforcedCapture());
    REQUIRE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());

    win.ClrEnforceCapture();
    // 0 .. visible
    // 0 .. enforced capture
    // 1 .. hidden behind dialog
    REQUIRE_FALSE(win.IsVisible());
    REQUIRE_FALSE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE(win.IsHiddenBehindDialog());
    REQUIRE_FALSE(win.IsCapturable());

    win.Show();
    // 1 .. visible
    // 0 .. enforced capture
    // 1 .. hidden behind dialog
    REQUIRE_FALSE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE_FALSE(win.HasEnforcedCapture());
    REQUIRE(win.IsHiddenBehindDialog());
    REQUIRE_FALSE(win.IsCapturable());

    win.SetEnforceCapture();
    // 1 .. visible
    // 1 .. enforced capture
    // 1 .. hidden behind dialog
    REQUIRE_FALSE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE(win.HasEnforcedCapture());
    REQUIRE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());

    win.ShowAfterDialog();
    // 1 .. visible
    // 1 .. enforced capture
    // 0 .. hidden behind dialog
    REQUIRE(win.IsVisible());
    REQUIRE(win.HasVisibleFlag());
    REQUIRE(win.HasEnforcedCapture());
    REQUIRE_FALSE(win.IsHiddenBehindDialog());
    REQUIRE(win.IsCapturable());
}

TEST_CASE("DoNotEnforceCapture_ScopeLock", "[window]") {
    window_t win(nullptr, Rect16(20, 20, 10, 10));

    SECTION("Disabled") {
        REQUIRE_FALSE(win.HasEnforcedCapture());

        {
            DoNotEnforceCapture_ScopeLock lock(win);
            REQUIRE_FALSE(win.HasEnforcedCapture());
        }

        // auto restored after end of the scope
        REQUIRE_FALSE(win.HasEnforcedCapture());
    }

    SECTION("Enabled") {
        win.SetEnforceCapture();
        REQUIRE(win.HasEnforcedCapture());

        {
            DoNotEnforceCapture_ScopeLock lock(win);
            REQUIRE_FALSE(win.HasEnforcedCapture());
        }

        // auto restored after end of the scope
        REQUIRE(win.HasEnforcedCapture());
    }
}
