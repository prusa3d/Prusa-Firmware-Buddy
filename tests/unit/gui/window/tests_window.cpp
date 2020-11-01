#include "catch2/catch.hpp"

#include "window.hpp"
#include "window_frame.hpp"
#include "sound_enum.h"
#include "window_dlg_popup.hpp"

void gui_timers_delete_by_window(window_t *pWin) {}
void gui_invalidate(void) {}
EventLock::EventLock(const char *event_method_name, window_t *sender, GUI_event_t event) {}
void Sound_Play(eSOUND_TYPE eSoundType) {}

struct MockScreen : public AddSuperWindow<window_frame_t> {
    window_t dummy0; // just so w0 is not first
    window_t w0;
    window_t w1;
    window_t w2;
    window_t w3;
    window_t dummy1; // just so w3 is not last

    MockScreen()
        : dummy0(this, Rect16(0, 0, 10, 10))
        , w0(this, Rect16(20, 20, 10, 10))
        , w1(this, Rect16(20, 40, 10, 10))
        , w2(this, Rect16(40, 20, 10, 10))
        , w3(this, Rect16(40, 40, 10, 10))
        , dummy1(this, Rect16(0, 0, 10, 10)) {}
};

TEST_CASE("Window tests", "[window]") {

    SECTION("window registration") {
        MockScreen screen;

        //check parrent
        REQUIRE(screen.dummy0.GetParent() == &screen);
        REQUIRE(screen.dummy1.GetParent() == &screen);
        REQUIRE(screen.w0.GetParent() == &screen);
        REQUIRE(screen.w1.GetParent() == &screen);
        REQUIRE(screen.w2.GetParent() == &screen);
        REQUIRE(screen.w3.GetParent() == &screen);

        //check linked list
        REQUIRE(screen.GetFirst() == &(screen.dummy0));
        REQUIRE(screen.GetLast() == &(screen.dummy1));
        REQUIRE(screen.GetLast()->GetNext() == nullptr);
        REQUIRE(screen.GetFirst()->GetNext() == &(screen.w0));
        REQUIRE(screen.w0.GetNext() == &(screen.w1));
        REQUIRE(screen.w1.GetNext() == &(screen.w2));
        REQUIRE(screen.w2.GetNext() == &(screen.w3));
        REQUIRE(screen.w3.GetNext() == screen.GetLast());
    }
}
