#include "catch2/catch.hpp"

#include "window.hpp"
#include "window_frame.hpp"
#include "sound_enum.h"

void gui_timers_delete_by_window(window_t *pWin) {}
void gui_invalidate(void) {}
EventLock::EventLock(const char *event_method_name, window_t *sender, GUI_event_t event) {}
void Sound_Play(eSOUND_TYPE eSoundType) {}

TEST_CASE("Window tests", "[window]") {

    SECTION("dummy test") {
        window_t w(nullptr, Rect16(0, 0, 10, 10));
        REQUIRE(true); // not clicked
    }
}
