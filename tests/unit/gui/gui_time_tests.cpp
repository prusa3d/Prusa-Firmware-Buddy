#include <cstdint>

#include "catch2/catch.hpp"
#include "gui_time.hpp"

uint32_t tick = 0;

TEST_CASE("gui time test", "[gui_time]") {

    SECTION("init state") {
        REQUIRE(gui::GetTick() == 0);
REQUIRE(gui::GetTickU64() == uint64_t(0));
REQUIRE(gui::GetTick_IgnoreTickLoop() == 0);
REQUIRE(gui::GetTick_ForceActualization() == 0);
}

SECTION("actualization") {
    tick = 100;
    gui::TickLoop();
    REQUIRE(gui::GetTick() == tick);
    REQUIRE(gui::GetTickU64() == uint64_t(tick));
    REQUIRE(gui::GetTick_IgnoreTickLoop() == tick);
    REQUIRE(gui::GetTick_ForceActualization() == tick);

    tick = 200;
    REQUIRE(gui::GetTick() == 100);
    REQUIRE(gui::GetTickU64() == uint64_t(100));
    REQUIRE(gui::GetTick_IgnoreTickLoop() == tick);

    REQUIRE(gui::GetTick_ForceActualization() == tick);
    REQUIRE(gui::GetTick() == tick);
    REQUIRE(gui::GetTickU64() == uint64_t(tick));
}

SECTION("overflow") {
    tick = 200;
    gui::TickLoop();
    tick = 100;
    gui::TickLoop();
    REQUIRE(gui::GetTickU64() == uint64_t(tick) + (uint64_t(1) << 32));

    tick = 50;
    REQUIRE(gui::GetTick_ForceActualization() == tick);
    REQUIRE(gui::GetTickU64() == uint64_t(tick) + (uint64_t(2) << 32));
}
}
;
