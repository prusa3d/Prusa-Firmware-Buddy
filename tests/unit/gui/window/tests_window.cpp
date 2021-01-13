#include "catch2/catch.hpp"

#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "cmsis_os.h" //HAL_GetTick
#include "mock_windows.hpp"
#include <memory>

//stubbed header does not have C linkage .. to be simpler
static uint32_t hal_tick = 0;
uint32_t HAL_GetTick() { return hal_tick; }

TEST_CASE("Window registration tests", "[window]") {
    MockScreen screen;
    Screens::Access()->Set(&screen); //instead of screen registration

    // initial screen check
    screen.BasicCheck();
    REQUIRE(screen.GetCapturedWindow() == &screen);

    SECTION("popup with no rectangle") {
        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        screen.BasicCheck(1); // basic check must pass, because rect is empty
        //popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);
    }

    SECTION("popup hiding w0 - w4") {
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect), string_view_utf8::MakeNULLSTR());
        //popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);
    }

    SECTION("msgbox with no rectangle") {
        MockMsgBox msgbox(Rect16(0, 0, 0, 0));
        screen.BasicCheck(0, 1); // basic check must pass, because rect is empty
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("msgbox hiding w0 - w4") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("popup with no rectangle + msgbox hiding w0 - w4") {
        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        screen.BasicCheck(1); // basic check must pass, because rect is empty
        //popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);

        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(popup->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox, popup);
    }

    SECTION("msgbox hiding w0 - w4 + popup with no rectangle") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);

        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        //popup is singleton must get its pointer from screen
        window_t *popup = msgbox.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(popup->GetParent() == &screen);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //popup does not claim capture
        screen.CheckOrderAndVisibility(&msgbox, popup);
    }

    SECTION("popup inside msgbox hiding w0 - w4") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect), string_view_utf8::MakeNULLSTR());
        //popup cannot open so test is same as if only msgbox is openned
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("msgbox closing popup hiding w0 - w4") {
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect), string_view_utf8::MakeNULLSTR());
        //popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);

        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        //popup must autoclose so test is same as if only msgbox is openned
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);
    }

    SECTION("live adj Z + M600") {
        //emulate by 2 nested msgboxes
        MockMsgBox msgbox0(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox0.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox0); //msgbox0 does claim capture
        screen.CheckOrderAndVisibility(&msgbox0);

        {
            MockMsgBox msgbox1(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
            REQUIRE(msgbox0.GetParent() == &screen);
            REQUIRE(msgbox1.GetParent() == &screen);
            REQUIRE(screen.GetCapturedWindow() == &msgbox1); //msgbox1 does claim capture
            screen.CheckOrderAndVisibility(&msgbox0, &msgbox1);
        }

        //retest after first msgbox is closed
        REQUIRE(msgbox0.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox0); //msgbox0 must get capture back
        screen.CheckOrderAndVisibility(&msgbox0);
    }

    SECTION("Unregister 2nd messagebox before 1st") {
        auto msgbox0 = std::make_unique<MockMsgBox>(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox0->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == msgbox0.get()); //msgbox0 does claim capture
        screen.CheckOrderAndVisibility(msgbox0.get());

        auto msgbox1 = std::make_unique<MockMsgBox>(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox0->GetParent() == &screen);
        REQUIRE(msgbox1->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == msgbox1.get()); //msgbox1 does claim capture
        screen.CheckOrderAndVisibility(msgbox0.get(), msgbox1.get());

        //destroy msgbox0 before msgbox1
        msgbox0.reset();

        //retest after second msgbox is closed
        REQUIRE(msgbox1->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == msgbox1.get()); //msgbox1 must remain captured
        screen.CheckOrderAndVisibility(msgbox1.get());
    }

    SECTION("strong dialog") {
        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(&strong);
        window_t::EventJogwheel(BtnState_t::Released); //unregister strong dialog
    }

    SECTION("2 strong dialogs") {
        MockStrongDialog &strong0 = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong0.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong0); //strong0 does claim capture
        screen.CheckOrderAndVisibility(&strong0);

        MockStrongDialog &strong1 = MockStrongDialog::ShowPrintFan();
        REQUIRE(strong0.GetParent() == &screen);
        REQUIRE(strong1.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong1); //strong1 does claim capture
        screen.CheckOrderAndVisibility(&strong0, &strong1);

        window_t::EventJogwheel(BtnState_t::Released); //unregister strong1
        REQUIRE(strong0.GetParent() == &screen);
        REQUIRE(strong1.GetParent() == nullptr);         // check parrent unregistration
        REQUIRE(screen.GetCapturedWindow() == &strong0); //strong1 resturns capture to strong1
        screen.CheckOrderAndVisibility(&strong0);
        window_t::EventJogwheel(BtnState_t::Released); //unregister strong0
    }

    SECTION("strong dialog + msgbox") {
        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(&strong);

        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong);   //strong cannot give capture to msgbox
        screen.CheckOrderAndVisibility(&msgbox, &strong); //strong must remain on top

        window_t::EventJogwheel(BtnState_t::Released); //unregister strong dialog
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //strong must give capture to message box upon destruction
        screen.CheckOrderAndVisibility(&msgbox);        //msgbox must remain
    }

    SECTION("msgbox + strong dialog") {
        MockMsgBox msgbox(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //msgbox does claim capture
        screen.CheckOrderAndVisibility(&msgbox);

        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(&msgbox, &strong);

        window_t::EventJogwheel(BtnState_t::Released); //unregister strong dialog
        REQUIRE(msgbox.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &msgbox); //strong must give capture to message box upon destruction
        screen.CheckOrderAndVisibility(&msgbox);        //msgbox must remain
    }

    SECTION("msgbox + strong dialog, destroy msgbox first") {
        auto msgbox = std::make_unique<MockMsgBox>(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect));
        REQUIRE(msgbox->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == msgbox.get()); //msgbox does claim capture
        screen.CheckOrderAndVisibility(msgbox.get());

        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(msgbox->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(msgbox.get(), &strong);

        //destroy msgbox before strong
        msgbox.reset();

        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(&strong);

        window_t::EventJogwheel(BtnState_t::Released); //unregister strong dialog
    }

    SECTION("popup with no rectangle + strong dialog") {
        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        screen.BasicCheck(1); // basic check must pass, because rect is empty
        //popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);

        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(popup->GetParent() == &screen);         // popup must remain
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(popup, &strong);

        window_t::EventJogwheel(BtnState_t::Released); //unregister strong dialog
        REQUIRE(popup->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);          //popup must remain
    }

    SECTION("strong dialog + popup with no rectangle") {
        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(&strong);

        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        window_t *popup = strong.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup, &strong);

        window_t::EventJogwheel(BtnState_t::Released); //unregister strong dialog
        REQUIRE(popup->GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);          //popup must remain
    }

    SECTION("normal window") {
        screen.BasicCheck();
        REQUIRE(screen.GetCapturedWindow() == &screen);
        screen.CaptureNormalWindow(screen.w0);
        screen.BasicCheck();
        REQUIRE(screen.GetCapturedWindow() == &screen.w0);
        screen.ReleaseCaptureOfNormalWindow();
    }

    SECTION("popup hiding w0 - w4 + normal window") {
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect), string_view_utf8::MakeNULLSTR());
        //popup is singleton must get its pointer from screen
        window_t *popup = screen.w_last.GetNext();
        REQUIRE_FALSE(popup == nullptr);
        REQUIRE(popup->GetType() == win_type_t::popup);
        REQUIRE(screen.GetCapturedWindow() == &screen); //popup does not claim capture
        screen.CheckOrderAndVisibility(popup);

        screen.CaptureNormalWindow(screen.w0);
        REQUIRE(screen.GetCapturedWindow() == &screen.w0);
        screen.ReleaseCaptureOfNormalWindow();
    }

    SECTION("strong dialog + normal window") {
        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(&strong);

        screen.CaptureNormalWindow(screen.w0);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong still has capture

        window_t::EventJogwheel(BtnState_t::Released);     //unregister strong dialog
        REQUIRE(screen.GetCapturedWindow() == &screen.w0); //capture must return to normal win
        screen.ReleaseCaptureOfNormalWindow();
    }

    SECTION("normal window + strong dialog") {
        screen.BasicCheck();
        REQUIRE(screen.GetCapturedWindow() == &screen);
        screen.CaptureNormalWindow(screen.w0);
        screen.BasicCheck();

        MockStrongDialog &strong = MockStrongDialog::ShowHotendFan();
        REQUIRE(strong.GetParent() == &screen);
        REQUIRE(screen.GetCapturedWindow() == &strong); //strong does claim capture
        screen.CheckOrderAndVisibility(&strong);

        window_t::EventJogwheel(BtnState_t::Released);     //unregister strong dialog
        REQUIRE(screen.GetCapturedWindow() == &screen.w0); //capture must return to normal win
        screen.ReleaseCaptureOfNormalWindow();
    }

    hal_tick = 1000;                                   //set openned on popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); //loop will initialize popup timeout
    hal_tick = 10000;                                  //timeout popup
    screen.ScreenEvent(&screen, GUI_event_t::LOOP, 0); //loop event will unregister popup

    //at the end of all sections screen must be returned to its original state
    screen.BasicCheck();
    REQUIRE(screen.GetCapturedWindow() == &screen);
}
