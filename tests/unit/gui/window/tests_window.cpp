#include <catch2/catch_test_macros.hpp>

#include "ScreenHandler.hpp"
#include "gui_time.hpp"
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

    SECTION("msgbox with no rectangle") {
        MockMsgBox msgbox(Rect16(0, 0, 0, 0));
        screen.BasicCheck(1); // basic check must pass, because rect is empty
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

    hal_tick = 1000; // set opened on popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); // loop will initialize popup timeout
    hal_tick = 10000; // timeout popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); // loop event will unregister popup

    // at the end of all sections screen must be returned to its original state
    screen.BasicCheck();
    REQUIRE(screen.GetCapturedWindow() == &screen);
}

// Helper: send a KNOB event to a screen's captured window, return whether accepted
static bool send_knob(screen_t &screen, int diff) {
    window_t *capture = screen.GetCapturedWindow();
    if (!capture) {
        return false;
    }
    GuiEventContext ctx { gui_event::KnobEvent { .diff = diff } };
    capture->WindowEvent(capture, GUI_event_t::KNOB, &ctx);
    return ctx.is_accepted();
}

TEST_CASE("Frame KNOB step-by-step focus rotation", "[window][knob]") {
    MockScreen screen;
    Screens::Access()->Set(&screen);

    // Enable children so they participate in focus navigation
    screen.w0.Enable();
    screen.w1.Enable();
    screen.w2.Enable();
    screen.w3.Enable();

    // Set initial focus on w0
    screen.w0.SetFocus();
    REQUIRE(window_t::GetFocusedWindow() == &screen.w0);

    SECTION("basic focus rotation forward") {
        bool accepted = send_knob(screen, 2);
        REQUIRE(accepted);
        REQUIRE(window_t::GetFocusedWindow() == &screen.w2);
    }

    SECTION("basic focus rotation backward") {
        screen.w3.SetFocus();
        REQUIRE(window_t::GetFocusedWindow() == &screen.w3);

        bool accepted = send_knob(screen, -2);
        REQUIRE(accepted);
        REQUIRE(window_t::GetFocusedWindow() == &screen.w1);
    }

    SECTION("boundary clamp — diff exceeds available children") {
        bool accepted = send_knob(screen, 20);
        REQUIRE(accepted);
        // Should move to last enabled child, not overshoot
        REQUIRE(window_t::GetFocusedWindow() == &screen.w3);
    }

    SECTION("no movement at forward boundary") {
        screen.w3.SetFocus();
        bool accepted = send_knob(screen, 1);
        // w3 is the last enabled child (w_last is not enabled), nothing to move to
        REQUIRE_FALSE(accepted);
        REQUIRE(window_t::GetFocusedWindow() == &screen.w3);
    }

    SECTION("no movement at backward boundary") {
        // w_first is not enabled, so w0 is the first enabled child
        bool accepted = send_knob(screen, -1);
        REQUIRE_FALSE(accepted);
        REQUIRE(window_t::GetFocusedWindow() == &screen.w0);
    }

    SECTION("diff = 1 moves exactly one step") {
        bool accepted = send_knob(screen, 1);
        REQUIRE(accepted);
        REQUIRE(window_t::GetFocusedWindow() == &screen.w1);
    }
}

TEST_CASE("Frame KNOB with children that accept steps", "[window][knob]") {
    // Create a screen-like frame with MockKnobAcceptor children
    // We need to use MockScreen as the base and add our own children
    MockScreen screen;
    Screens::Access()->Set(&screen);

    // Sub frame with children that limit amount of used knob events (eg.
    // something like two menus side by side, each with limited amount of
    // items).
    window_frame_t frame(nullptr, GuiDefaults::RectScreen);

    MockKnobAcceptor child_a(&frame, Rect16(10, 10, 10, 10));
    child_a.Enable();
    MockKnobAcceptor child_b(&frame, Rect16(10, 30, 10, 10));
    child_b.Enable();

    child_a.SetFocus();
    REQUIRE(window_t::GetFocusedWindow() == &child_a);

    SECTION("child consumes all steps") {
        child_a.remaining_accepts = 5;

        GuiEventContext ctx { gui_event::KnobEvent { .diff = 3 } };
        frame.WindowEvent(&frame, GUI_event_t::KNOB, &ctx);

        REQUIRE(ctx.is_accepted());
        REQUIRE(child_a.total_accepted == 3);
        REQUIRE(child_a.remaining_accepts == 2);
        // Focus stays on child_a since it consumed everything
        REQUIRE(window_t::GetFocusedWindow() == &child_a);
    }

    SECTION("child partially consumes, then focus transfers") {
        child_a.remaining_accepts = 2; // accepts 2, rejects 3rd
        child_b.remaining_accepts = 5;

        GuiEventContext ctx { gui_event::KnobEvent { .diff = 5 } };
        frame.WindowEvent(&frame, GUI_event_t::KNOB, &ctx);

        REQUIRE(ctx.is_accepted());
        // child_a consumed 2, then rejected
        REQUIRE(child_a.total_accepted == 2);
        // 1 step used for focus change to child_b
        // remaining 2 steps consumed by child_b
        REQUIRE(child_b.total_accepted == 2);
        REQUIRE(window_t::GetFocusedWindow() == &child_b);
    }

    SECTION("child rejects immediately, focus transfers") {
        child_a.remaining_accepts = 0;
        child_b.remaining_accepts = 5;

        GuiEventContext ctx { gui_event::KnobEvent { .diff = 3 } };
        frame.WindowEvent(&frame, GUI_event_t::KNOB, &ctx);

        REQUIRE(ctx.is_accepted());
        REQUIRE(child_a.total_accepted == 0);
        // 1 step for focus change, 2 consumed by child_b
        REQUIRE(child_b.total_accepted == 2);
        REQUIRE(window_t::GetFocusedWindow() == &child_b);
    }

    SECTION("both children reject — single child at boundary") {
        child_a.remaining_accepts = 0;
        child_b.remaining_accepts = 0;

        GuiEventContext ctx { gui_event::KnobEvent { .diff = 3 } };
        frame.WindowEvent(&frame, GUI_event_t::KNOB, &ctx);

        // child_a rejects → focus changes to child_b (1 consumed)
        // child_b rejects → no more siblings → stop
        REQUIRE(ctx.is_accepted()); // focus did change
        REQUIRE(window_t::GetFocusedWindow() == &child_b);
    }

    SECTION("no focused child") {
        window_t::ResetFocusedWindow();

        GuiEventContext ctx { gui_event::KnobEvent { .diff = 3 } };
        frame.WindowEvent(&frame, GUI_event_t::KNOB, &ctx);

        REQUIRE_FALSE(ctx.is_accepted());
    }

    SECTION("negative diff — backward focus transfer") {
        child_b.SetFocus();
        child_b.remaining_accepts = 1;
        child_a.remaining_accepts = 5;

        GuiEventContext ctx { gui_event::KnobEvent { .diff = -4 } };
        frame.WindowEvent(&frame, GUI_event_t::KNOB, &ctx);

        REQUIRE(ctx.is_accepted());
        REQUIRE(child_b.total_accepted == 1); // consumed 1 backward
        // 1 step for focus change to child_a
        // 2 remaining consumed by child_a
        REQUIRE(child_a.total_accepted == 2);
        REQUIRE(window_t::GetFocusedWindow() == &child_a);
    }
}

TEST_CASE("Capturable test, all combinations", "[window]") {
    BasicWindow win(nullptr, Rect16(20, 20, 10, 10));

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
    BasicWindow win(nullptr, Rect16(20, 20, 10, 10));

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
