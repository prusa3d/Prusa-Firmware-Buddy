#include "catch2/catch.hpp"

#include "window.hpp"
#include "window_frame.hpp"
#include "sound_enum.h"
#include "window_dlg_popup.hpp"
#include "ScreenHandler.hpp"
#include "IDialog.hpp"
#include "cmsis_os.h" //HAL_GetTick

void gui_timers_delete_by_window(window_t *pWin) {}
void gui_invalidate(void) {}
EventLock::EventLock(const char *event_method_name, window_t *sender, GUI_event_t event) {}
void Sound_Play(eSOUND_TYPE eSoundType) {}
void gui_loop() {}

//stubbed header does not have C linkage .. to be simpler
uint32_t HAL_GetTick() { return 0; }

struct MockScreen : public AddSuperWindow<window_frame_t> {
    window_t w_first; // just so w0 is not first
    window_t w0;
    window_t w1;
    window_t w2;
    window_t w3;
    window_t w_last; // just so w3 is not last

    MockScreen()
        : w_first(this, Rect16(0, 0, 10, 10))
        , w0(this, Rect16(20, 20, 10, 10))
        , w1(this, Rect16(20, 40, 10, 10))
        , w2(this, Rect16(40, 20, 10, 10))
        , w3(this, Rect16(40, 40, 10, 10))
        , w_last(this, Rect16(0, 0, 10, 10)) {}
};
enum class has_dialog_t : bool { no,
    yes };

void window_parrent_check(MockScreen &screen) {
    //check parrent
    REQUIRE(screen.w_first.GetParent() == &screen);
    REQUIRE(screen.w_last.GetParent() == &screen);
    REQUIRE(screen.w0.GetParent() == &screen);
    REQUIRE(screen.w1.GetParent() == &screen);
    REQUIRE(screen.w2.GetParent() == &screen);
    REQUIRE(screen.w3.GetParent() == &screen);
}

void window_linked_list_check(MockScreen &screen, has_dialog_t has_dialog) {
    //check linked list
    REQUIRE(screen.GetFirst() == &(screen.w_first));
    REQUIRE(screen.GetLast()->GetNext() == nullptr);
    REQUIRE(screen.GetFirst()->GetNext() == &(screen.w0));
    REQUIRE(screen.w0.GetNext() == &(screen.w1));
    REQUIRE(screen.w1.GetNext() == &(screen.w2));
    REQUIRE(screen.w2.GetNext() == &(screen.w3));
    REQUIRE(screen.w3.GetNext() == &(screen.w_last));
    REQUIRE((screen.w_last.GetNext() == nullptr) != bool(has_dialog));
    REQUIRE((screen.GetLast() == &(screen.w_last)) != bool(has_dialog));
}

void basic_basic_screen_check(MockScreen &screen, has_dialog_t has_dialog) {
    //check parrent
    window_parrent_check(screen);
    screen.SetCapture();

    //check IsHiddenBehindDialog()
    REQUIRE_FALSE(screen.w_first.IsHiddenBehindDialog());
    REQUIRE_FALSE(screen.w_last.IsHiddenBehindDialog());
    REQUIRE_FALSE(screen.w0.IsHiddenBehindDialog());
    REQUIRE_FALSE(screen.w1.IsHiddenBehindDialog());
    REQUIRE_FALSE(screen.w2.IsHiddenBehindDialog());
    REQUIRE_FALSE(screen.w3.IsHiddenBehindDialog());

    //check linked list
    window_linked_list_check(screen, has_dialog);
}

TEST_CASE("Window registration tests", "[window]") {
    MockScreen screen;
    Screens::Access()->Set(&screen); //instead of screen registration

    SECTION("initial screen check") {
        basic_basic_screen_check(screen, has_dialog_t::no);
        REQUIRE(window_t::GetCapturedWindow() == &screen);
    }

    SECTION("popup with no rectangle") {
        window_dlg_popup_t::Show(Rect16(), string_view_utf8::MakeNULLSTR());
        basic_basic_screen_check(screen, has_dialog_t::yes);
        REQUIRE(screen.GetLast() == screen.w_last.GetNext());
        // REQUIRE_FALSE(window_t::GetCapturedWindow() == &screen);
    }

    SECTION("popup hiding w0 - w4") {
        window_dlg_popup_t::Show(Rect16::Merge_ParamPack(screen.w0.rect, screen.w1.rect, screen.w2.rect, screen.w3.rect), string_view_utf8::MakeNULLSTR());
        //check parrent
        window_parrent_check(screen);

        //check IsHiddenBehindDialog()
        REQUIRE_FALSE(screen.w_first.IsHiddenBehindDialog());
        REQUIRE_FALSE(screen.w_last.IsHiddenBehindDialog());
        REQUIRE(screen.w0.IsHiddenBehindDialog());
        REQUIRE(screen.w1.IsHiddenBehindDialog());
        REQUIRE(screen.w2.IsHiddenBehindDialog());
        REQUIRE(screen.w3.IsHiddenBehindDialog());

        //check linked list
        window_linked_list_check(screen, has_dialog_t::yes);

        REQUIRE(screen.GetLast() == screen.w_last.GetNext());
        //REQUIRE_FALSE(window_t::GetCapturedWindow() == &screen);
    }
}
