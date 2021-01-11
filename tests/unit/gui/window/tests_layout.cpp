#include "catch2/catch.hpp"

#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "cmsis_os.h" //HAL_GetTick
#include "mock_windows.hpp"
#include <memory>

void gui_timers_delete_by_window(window_t *pWin) {}
void gui_invalidate(void) {}
EventLock::EventLock(const char *event_method_name, window_t *sender, GUI_event_t event) {}
void Sound_Play(eSOUND_TYPE eSoundType) {}
void gui_loop() {}
extern "C" void marlin_notify_server_about_encoder_move() {}
extern "C" void marlin_notify_server_about_konb_click() {}

//stubbed header does not have C linkage .. to be simpler
static uint32_t hal_tick = 0;
uint32_t HAL_GetTick() { return hal_tick; }

TEST_CASE("Window registration tests", "[window]") {

    REQUIRE(true);

    SECTION("xxx") {

        REQUIRE(true);
    }
}
