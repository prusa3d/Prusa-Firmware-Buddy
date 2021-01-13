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

    REQUIRE(true);

    SECTION("xxx") {

        REQUIRE(true);
    }
}
